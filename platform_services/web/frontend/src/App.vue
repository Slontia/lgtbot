<script setup lang="ts">
import { Moon, Sunny } from '@element-plus/icons-vue'
import { computed, onMounted, ref, watch } from 'vue'
import { RouterLink, RouterView, useRouter } from 'vue-router'
import { useAuthStore } from './stores/auth'
import { useThemeStore } from './stores/theme'

const base = import.meta.env.BASE_URL
const logoSrc = computed(() => `${base}logo.svg`)

const auth = useAuthStore()
const theme = useThemeStore()
const router = useRouter()

const me = ref<{ display_name?: string; avatar_url?: string } | null>(null)

const avatarText = computed(() => {
  const n = me.value?.display_name || auth.username || ''
  return n.slice(0, 1).toUpperCase() || '?'
})

async function loadMe() {
  if (!auth.token) {
    me.value = null
    return
  }
  try {
    const res = await fetch('/api/me', {
      headers: { Authorization: `Bearer ${auth.token}` },
    })
    if (res.ok) {
      me.value = (await res.json()) as { display_name?: string; avatar_url?: string }
    }
  } catch {
    me.value = null
  }
}

onMounted(loadMe)
watch(() => auth.token, loadMe)

function onDropdown(cmd: string) {
  if (cmd === 'logout') {
    auth.logout()
    me.value = null
    router.push('/login')
    return
  }
  if (cmd === 'profile') {
    router.push('/profile')
  }
}
</script>

<template>
  <div class="app-shell">
    <header class="top-nav">
      <RouterLink to="/" class="brand" aria-label="首页">
        <img :src="logoSrc" class="logo-img" alt="LGTBot" width="40" height="46" />
      </RouterLink>
      <div class="nav-right">
        <el-button
          class="theme-toggle"
          circle
          text
          :aria-label="theme.isDark ? '切换浅色模式' : '切换深色模式'"
          @click="theme.toggle()"
        >
          <el-icon :size="22">
            <Moon v-if="!theme.isDark" />
            <Sunny v-else />
          </el-icon>
        </el-button>
        <template v-if="auth.isLoggedIn">
          <el-dropdown trigger="click" @command="onDropdown">
            <span class="avatar-trigger">
              <el-avatar :size="40" :src="me?.avatar_url || undefined">
                {{ avatarText }}
              </el-avatar>
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="profile">个人信息</el-dropdown-item>
                <el-dropdown-item divided command="logout">登出</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </template>
        <RouterLink v-else to="/login" class="login-link">登录</RouterLink>
      </div>
    </header>
    <main class="main">
      <RouterView />
    </main>
  </div>
</template>

<style scoped>
.app-shell {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}
.top-nav {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 10px 20px;
  border-bottom: 1px solid var(--el-border-color);
  background: var(--el-bg-color);
}
.brand {
  display: flex;
  align-items: center;
  text-decoration: none;
}
.logo-img {
  display: block;
  height: 40px;
  width: auto;
}
.nav-right {
  display: flex;
  align-items: center;
  gap: 12px;
}
.theme-toggle {
  color: var(--el-text-color-primary);
  padding: 8px;
}
.theme-toggle:hover {
  color: var(--el-color-primary);
}
.avatar-trigger {
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  outline: none;
}
.login-link {
  color: var(--el-color-primary);
  font-size: 14px;
  text-decoration: none;
}
.login-link:hover {
  opacity: 0.85;
}
.main {
  flex: 1;
  padding: 20px;
  max-width: 1280px;
  margin: 0 auto;
  width: 100%;
  box-sizing: border-box;
}
</style>
