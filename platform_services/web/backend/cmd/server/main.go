package main

import (
	"context"
	"flag"
	"log"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"

	"lgtbot/web/backend/internal/api"
	"lgtbot/web/backend/internal/auth"
	"lgtbot/web/backend/internal/bot"
	"lgtbot/web/backend/internal/hub"
)

func getenv(key, def string) string {
	v := os.Getenv(key)
	if v == "" {
		return def
	}
	return v
}

func main() {
	gamePathFlag := flag.String("game-path", "",
		"Directory that contains game plugin folders (same as lgtbot_grpc_server — use absolute path when possible); overrides LGTBOT_GAME_PATH if set.")
	imagePathFlag := flag.String("image-path", "",
		"Avatar / core image filesystem root (same as lgtbot_grpc_server LGTBOT_IMAGE_PATH / --image-path); overrides LGTBOT_IMAGE_PATH if set.")

	flag.Parse()

	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()

	grpcAddr := getenv("LGTBOT_GRPC_ADDR", "unix:///tmp/lgtbot.sock")
	httpAddr := getenv("HTTP_ADDR", ":8080")
	jwtSecret := getenv("JWT_SECRET", "dev-change-me")
	platform := getenv("LGTBOT_PLATFORM", "web")
	dbPath := getenv("WEB_SQLITE_PATH", "./data/web_users.db")

	gamePathRaw := strings.TrimSpace(*gamePathFlag)
	if gamePathRaw == "" {
		gamePathRaw = getenv("LGTBOT_GAME_PATH", "plugins")
	}
	gamePluginsAbs, err := filepath.Abs(filepath.Clean(gamePathRaw))
	if err != nil {
		log.Fatalf("game path: resolve absolute path: %v", err)
	}
	fi, err := os.Stat(gamePluginsAbs)
	if err != nil {
		log.Fatalf("game plugins path not accessible (%s): %v", gamePluginsAbs, err)
	}
	if !fi.IsDir() {
		log.Fatalf("game plugins path is not a directory: %s", gamePluginsAbs)
	}

	imagePathRaw := strings.TrimSpace(*imagePathFlag)
	if imagePathRaw == "" {
		imagePathRaw = getenv("LGTBOT_IMAGE_PATH", "/tmp/lgtbot_images")
	}
	imageRootAbs, err := filepath.Abs(filepath.Clean(imagePathRaw))
	if err != nil {
		log.Fatalf("image path: resolve absolute path: %v", err)
	}

	if err := os.MkdirAll("./data", 0755); err != nil {
		log.Fatal(err)
	}

	store, err := auth.OpenSQLite(dbPath)
	if err != nil {
		log.Fatal(err)
	}
	bc, err := bot.Dial(ctx, grpcAddr)
	if err != nil {
		log.Fatal(err)
	}
	defer bc.Close()

	waitSec := 180
	if v := getenv("LGTBOT_CORE_WAIT_SEC", ""); v != "" {
		if n, err := strconv.Atoi(v); err == nil && n > 0 {
			waitSec = n
		}
	}
	waitCtx, waitCancel := context.WithTimeout(ctx, time.Duration(waitSec)*time.Second)
	if err := bot.WaitForCore(waitCtx, bc); err != nil {
		waitCancel()
		log.Fatalf("core_service not ready (start lgtbot_grpc_server in another terminal; LGTBOT_GRPC_ADDR must match socket/TCP address): %v", err)
	}
	waitCancel()

	pushCh, err := bc.SubscribeAsync(ctx, platform)
	if err != nil {
		log.Fatal(err)
	}
	h := hub.NewHub(bc, platform, pushCh)
	go h.Run(ctx)

	apiSrv := api.New(store, bc, h, jwtSecret, platform, gamePluginsAbs, imageRootAbs)

	log.Printf("listening HTTP %s (grpc %s platform=%s game_plugins=%s image_root=%s)", httpAddr, grpcAddr, platform, gamePluginsAbs, imageRootAbs)
	srv := &http.Server{
		Addr:              httpAddr,
		Handler:           cors(apiSrv.Router),
		ReadHeaderTimeout: 10 * time.Second,
	}
	go func() {
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatal(err)
		}
	}()

	<-ctx.Done()
	shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	_ = srv.Shutdown(shutdownCtx)
}

func cors(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next.ServeHTTP(w, r)
	})
}
