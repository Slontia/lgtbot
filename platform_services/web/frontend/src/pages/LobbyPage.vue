<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { ElMessage } from 'element-plus'
import { useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'

interface GameRow {
  name: string
  developer?: string
  description?: string
  min_players?: number
  max_players?: number
}

const auth = useAuthStore()
const router = useRouter()
const games = ref<GameRow[]>([])
const err = ref('')
const starting = ref<string | null>(null)

onMounted(async () => {
  try {
    const res = await fetch('/api/games')
    if (!res.ok) throw new Error(await res.text())
    const data = (await res.json()) as { games?: GameRow[] }
    games.value = data.games ?? []
  } catch (e) {
    err.value = e instanceof Error ? e.message : String(e)
  }
})

async function startSolo(row: GameRow) {
  if (!auth.token) {
    ElMessage.warning('请先登录')
    return
  }
  starting.value = row.name
  try {
    const res = await fetch('/api/game/start', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        Authorization: `Bearer ${auth.token}`,
      },
      body: JSON.stringify({ game_name: row.name, solo: true }),
    })
    const txt = await res.text()
    if (!res.ok) throw new Error(txt)
    ElMessage.success(`已向 bot 发送开局：${row.name}`)
    await router.push('/game')
  } catch (e) {
    ElMessage.error(e instanceof Error ? e.message : String(e))
  } finally {
    starting.value = null
  }
}
</script>

<template>
  <el-card>
    <template #header>游戏列表（来自 core gRPC）</template>
    <el-alert v-if="err" type="warning" :title="err" show-icon />
    <el-table v-else :data="games" stripe empty-text="暂无数据（请先启动 lgtbot_grpc_server）">
      <el-table-column prop="name" label="名称" width="180" />
      <el-table-column prop="description" label="说明" />
      <el-table-column prop="min_players" label="最少人数" width="100" />
      <el-table-column prop="max_players" label="最多人数" width="100" />
      <el-table-column label="操作" width="140" fixed="right">
        <template #default="{ row }">
          <el-button
            type="primary"
            size="small"
            :loading="starting === row.name"
            @click="startSolo(row)"
          >
            单机开局
          </el-button>
        </template>
      </el-table-column>
    </el-table>
  </el-card>
</template>
