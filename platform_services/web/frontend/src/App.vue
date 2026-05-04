<script setup lang="ts">
import { RouterLink, RouterView } from 'vue-router'
import { useAuthStore } from './stores/auth'

const auth = useAuthStore()
function logout() {
  auth.logout()
  window.location.href = '/login'
}
</script>

<template>
  <el-menu mode="horizontal" :ellipsis="false" class="nav">
    <el-menu-item index="brand">
      <RouterLink to="/"><strong>LGTBot</strong></RouterLink>
    </el-menu-item>
    <template v-if="auth.isLoggedIn">
      <el-menu-item index="lobby"><RouterLink to="/lobby">大厅</RouterLink></el-menu-item>
      <el-menu-item index="game"><RouterLink to="/game">游戏</RouterLink></el-menu-item>
      <el-menu-item index="history"><RouterLink to="/history">历史</RouterLink></el-menu-item>
      <el-menu-item index="profile"><RouterLink to="/profile">个人</RouterLink></el-menu-item>
      <el-menu-item index="logout" @click="logout">退出</el-menu-item>
    </template>
    <el-menu-item v-else index="login"><RouterLink to="/login">登录</RouterLink></el-menu-item>
  </el-menu>
  <RouterView />
</template>

<style scoped>
.nav {
  margin-bottom: 1rem;
}
</style>
