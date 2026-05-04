module lgtbot/web/backend

go 1.25.0

require (
	github.com/golang-jwt/jwt/v5 v5.3.1
	github.com/gorilla/mux v1.8.1
	github.com/gorilla/websocket v1.5.3
	golang.org/x/crypto v0.50.0
	google.golang.org/grpc v1.80.0
	gorm.io/driver/sqlite v1.6.0
	gorm.io/gorm v1.31.1
	lgtbot/proto v0.0.0
)

replace lgtbot/proto => ../../../proto/go

require (
	github.com/jinzhu/inflection v1.0.0 // indirect
	github.com/jinzhu/now v1.1.5 // indirect
	github.com/mattn/go-sqlite3 v1.14.22 // indirect
	golang.org/x/net v0.52.0 // indirect
	golang.org/x/sys v0.43.0 // indirect
	golang.org/x/text v0.36.0 // indirect
	google.golang.org/genproto/googleapis/rpc v0.0.0-20260120221211-b8f7ae30c516 // indirect
	google.golang.org/protobuf v1.36.11 // indirect
)
