package bot

import (
	"context"
	"fmt"
	"log"
	"time"
)

// WaitForCore polls GetGameList until success or ctx deadline (grpc lazy-connect makes Dial succeed without core).
func WaitForCore(ctx context.Context, c *BotClient) error {
	ticker := time.NewTicker(250 * time.Millisecond)
	defer ticker.Stop()
	var lastErr error
	var lastLog time.Time
	start := time.Now()
	for {
		select {
		case <-ctx.Done():
			if lastErr != nil {
				return fmt.Errorf("%w (last error: %v)", ctx.Err(), lastErr)
			}
			return ctx.Err()
		case <-ticker.C:
			pingCtx, cancel := context.WithTimeout(ctx, 8*time.Second)
			_, err := c.GetGameList(pingCtx)
			cancel()
			if err == nil {
				log.Printf("core gRPC ready (after %v)", time.Since(start).Round(time.Second))
				return nil
			}
			lastErr = err
			if lastLog.IsZero() || time.Since(lastLog) >= 10*time.Second {
				log.Printf("waiting for core gRPC... %v", err)
				lastLog = time.Now()
			}
		}
	}
}
