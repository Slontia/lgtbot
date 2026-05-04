<script setup lang="ts">
import { useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'

const auth = useAuthStore()
const router = useRouter()

function logout() {
  auth.logout()
  router.push('/login')
}
</script>

<template>
  <el-descriptions title="账号" border>
    <el-descriptions-item label="用户名">{{ auth.username }}</el-descriptions-item>
  </el-descriptions>
  <el-button style="margin-top: 16px" @click="logout">退出登录</el-button>
</template>
