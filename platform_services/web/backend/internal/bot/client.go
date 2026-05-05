package bot

import (
	"context"
	"errors"
	"io"
	"log"
	"sync"
	"time"

	"lgtbot/proto/pb"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

// BotClient wraps the core_service LGTBotService gRPC API (unix or TCP).
type BotClient struct {
	conn   *grpc.ClientConn
	client pb.LGTBotServiceClient
}

func Dial(ctx context.Context, target string) (*BotClient, error) {
	conn, err := grpc.NewClient(target, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return nil, err
	}
	return &BotClient{conn: conn, client: pb.NewLGTBotServiceClient(conn)}, nil
}

func (c *BotClient) Close() error {
	if c.conn == nil {
		return nil
	}
	return c.conn.Close()
}

func (c *BotClient) HandlePrivateRequest(ctx context.Context, platform, uid, text string) error {
	_, err := c.client.HandlePrivateRequest(ctx, &pb.PrivateRequest{
		Platform: platform, PlatformUserId: uid, Text: text,
	})
	return err
}

// Subscribe runs one streaming RPC for the platform and pushes events to out until ctx done or stream ends.
func (c *BotClient) Subscribe(ctx context.Context, platform string, out chan<- *pb.PushEvent) error {
	var stream grpc.ServerStreamingClient[pb.PushEvent]
	var err error
	const maxAttempts = 120
	for attempt := 0; attempt < maxAttempts; attempt++ {
		stream, err = c.client.Subscribe(ctx, &pb.SubscribeRequest{Platform: platform})
		if err == nil {
			break
		}
		if ctx.Err() != nil {
			return ctx.Err()
		}
		time.Sleep(100 * time.Millisecond)
	}
	if err != nil {
		return err
	}
	for {
		ev, err := stream.Recv()
		if errors.Is(err, io.EOF) {
			return nil
		}
		if err != nil {
			return err
		}
		select {
		case <-ctx.Done():
			return ctx.Err()
		case out <- ev:
		}
	}
}

func (c *BotClient) SubscribeAsync(ctx context.Context, platform string) (<-chan *pb.PushEvent, error) {
	ch := make(chan *pb.PushEvent, 32)
	var once sync.Once
	closeCh := func() { once.Do(func() { close(ch) }) }
	go func() {
		defer closeCh()
		if err := c.Subscribe(ctx, platform, ch); err != nil && ctx.Err() == nil {
			log.Printf("bot Subscribe ended: %v", err)
		}
	}()
	return ch, nil
}

func (c *BotClient) UpdateUserInfo(ctx context.Context, platform, uid, name string, avatar []byte) error {
	_, err := c.client.UpdateUserInfo(ctx, &pb.UpdateUserInfoRequest{
		Platform: platform, PlatformUserId: uid, DisplayName: name, AvatarPng: avatar,
	})
	return err
}

func (c *BotClient) GetGameList(ctx context.Context) ([]*pb.GameInfo, error) {
	resp, err := c.client.GetGameList(ctx, &pb.GetGameListRequest{})
	if err != nil {
		return nil, err
	}
	return resp.Games, nil
}

func (c *BotClient) GetUserInfo(ctx context.Context, platform, platformUserID string) (*pb.GetUserInfoResponse, error) {
	return c.client.GetUserInfo(ctx, &pb.GetUserInfoRequest{
		Platform: platform, PlatformUserId: platformUserID,
	})
}

func (c *BotClient) StartGame(ctx context.Context, platform, uid, game string, solo bool) error {
	resp, err := c.client.StartGame(ctx, &pb.StartGameRequest{
		Platform: platform, PlatformUserId: uid, GameName: game, Solo: solo,
	})
	if err != nil {
		return err
	}
	if resp != nil && resp.ErrCode != 0 {
		return errors.New(resp.ErrMsg)
	}
	return nil
}

func (c *BotClient) WebListGameMatches(ctx context.Context, platform, gameName string, offset, limit uint32) (*pb.WebListGameMatchesResponse, error) {
	return c.client.WebListGameMatches(ctx, &pb.WebListGameMatchesRequest{
		Platform: platform, GameName: gameName, Offset: offset, Limit: limit,
	})
}

func (c *BotClient) WebGetGameRule(ctx context.Context, gameName string) (*pb.WebGetGameRuleResponse, error) {
	return c.client.WebGetGameRule(ctx, &pb.WebGetGameRuleRequest{GameName: gameName})
}

func (c *BotClient) WebGetGameAchievements(ctx context.Context, gameName, viewerPlatformUserID string) (*pb.WebGetGameAchievementsResponse, error) {
	return c.client.WebGetGameAchievements(ctx, &pb.WebGetGameAchievementsRequest{
		GameName: gameName, ViewerPlatformUserId: viewerPlatformUserID,
	})
}

func (c *BotClient) WebGetGameRankings(ctx context.Context, platform, gameName string, timeRange pb.WebTimeRange, topN uint32) (*pb.WebGetGameRankingsResponse, error) {
	return c.client.WebGetGameRankings(ctx, &pb.WebGetGameRankingsRequest{
		Platform: platform, GameName: gameName, TimeRange: timeRange, TopN: topN,
	})
}

func (c *BotClient) WebUserProfile(ctx context.Context, platform, platformUserID string, timeRange pb.WebTimeRange) (*pb.WebUserProfileResponse, error) {
	return c.client.WebUserProfile(ctx, &pb.WebUserProfileRequest{
		Platform: platform, PlatformUserId: platformUserID, TimeRange: timeRange,
	})
}
