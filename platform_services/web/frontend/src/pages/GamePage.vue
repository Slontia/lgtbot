<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useWebSocket } from '../composables/useWebSocket'

const { connected, lastMessages, connect, sendGameText } = useWebSocket()
const line = ref('')

onMounted(() => {
  connect()
})

function send() {
  const text = line.value.trim()
  if (!text) return
  sendGameText(text)
  line.value = ''
}
</script>

<template>
  <el-card>
    <template #header>
      游戏控制台
      <el-tag :type="connected ? 'success' : 'info'" size="small" style="margin-left: 8px">
        WS {{ connected ? '已连接' : '未连接' }}
      </el-tag>
    </template>
    <p class="hint">通过 WebSocket 向 bot 发送私聊指令（对应 Go → core HandlePrivateRequest）。</p>
    <el-input v-model="line" placeholder="例如：帮助" @keyup.enter="send">
      <template #append>
        <el-button type="primary" @click="send">发送</el-button>
      </template>
    </el-input>
    <el-scrollbar height="200px" style="margin-top: 12px">
      <div v-for="(m, i) in lastMessages" :key="i" class="msg">{{ m }}</div>
    </el-scrollbar>
  </el-card>
</template>

<style scoped>
.hint {
  color: var(--el-text-color-secondary);
  margin-bottom: 8px;
}
.msg {
  padding: 4px 0;
  border-bottom: 1px solid var(--el-border-color-lighter);
}
</style>
