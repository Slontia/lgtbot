<script setup lang="ts">
import { onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'

const auth = useAuthStore()
const router = useRouter()
onMounted(() => {
  if (auth.isLoggedIn) router.replace('/lobby')
})
</script>

<template>
  <el-card shadow="never" style="max-width: 640px; margin: 2rem auto">
    <h1>LGTBot Web</h1>
    <p>大厅对战与单机开局请在登录后使用。</p>
    <router-link v-if="!auth.isLoggedIn" to="/login">
      <el-button type="primary">前往登录</el-button>
    </router-link>
  </el-card>
</template>
