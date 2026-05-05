<script setup lang="ts">
import { ref } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'

const route = useRoute()
const router = useRouter()
const auth = useAuthStore()

const username = ref('')
const password = ref('')
const loading = ref(false)
const errorMsg = ref('')

async function doLogin() {
  errorMsg.value = ''
  loading.value = true
  try {
    const res = await fetch('/api/auth/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username: username.value, password: password.value }),
    })
    if (!res.ok) throw new Error(await res.text())
    const data = (await res.json()) as { token: string; username: string }
    auth.setSession(data.token, data.username)
    const redir = typeof route.query.redirect === 'string' ? route.query.redirect : '/'
    router.push(redir)
  } catch (e) {
    errorMsg.value = e instanceof Error ? e.message : 'login failed'
  } finally {
    loading.value = false
  }
}

async function doRegister() {
  errorMsg.value = ''
  loading.value = true
  try {
    const res = await fetch('/api/auth/register', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ username: username.value, password: password.value }),
    })
    if (!res.ok) throw new Error(await res.text())
    const data = (await res.json()) as { token: string; username: string }
    auth.setSession(data.token, data.username)
    router.push('/')
  } catch (e) {
    errorMsg.value = e instanceof Error ? e.message : 'register failed'
  } finally {
    loading.value = false
  }
}
</script>

<template>
  <el-card style="max-width: 420px; margin: 2rem auto">
    <template #header>账号</template>
    <el-alert v-if="errorMsg" type="error" :title="errorMsg" show-icon style="margin-bottom: 12px" />
    <el-form label-position="top">
      <el-form-item label="用户名">
        <el-input v-model="username" autocomplete="username" />
      </el-form-item>
      <el-form-item label="密码">
        <el-input v-model="password" type="password" autocomplete="current-password" />
      </el-form-item>
      <el-form-item>
        <el-button type="primary" :loading="loading" @click="doLogin">登录</el-button>
        <el-button :loading="loading" @click="doRegister">注册</el-button>
      </el-form-item>
    </el-form>
  </el-card>
</template>
