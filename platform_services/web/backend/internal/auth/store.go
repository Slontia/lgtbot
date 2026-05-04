package auth

import (
	"errors"
	"time"

	"golang.org/x/crypto/bcrypt"
	"gorm.io/driver/sqlite"
	"gorm.io/gorm"
)

type WebUser struct {
	Username     string `gorm:"primaryKey"`
	PasswordHash string `gorm:"not null"`
	CreatedAt    time.Time
}

type Store struct {
	db *gorm.DB
}

func OpenSQLite(path string) (*Store, error) {
	db, err := gorm.Open(sqlite.Open(path), &gorm.Config{})
	if err != nil {
		return nil, err
	}
	if err := db.AutoMigrate(&WebUser{}); err != nil {
		return nil, err
	}
	return &Store{db: db}, nil
}

func (s *Store) Register(username, password string) error {
	if username == "" || password == "" {
		return errors.New("empty username or password")
	}
	hash, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	if err != nil {
		return err
	}
	u := WebUser{Username: username, PasswordHash: string(hash)}
	return s.db.Create(&u).Error
}

func (s *Store) Verify(username, password string) error {
	var u WebUser
	if err := s.db.First(&u, "username = ?", username).Error; err != nil {
		return err
	}
	return bcrypt.CompareHashAndPassword([]byte(u.PasswordHash), []byte(password))
}
