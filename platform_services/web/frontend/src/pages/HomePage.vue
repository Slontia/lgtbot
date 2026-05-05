<script setup lang="ts">
import { onMounted, ref } from 'vue'
import GameCard, { type GameRow } from '../components/GameCard.vue'

const games = ref<GameRow[]>([])
const err = ref('')

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
</script>

<template>
  <div class="home">
    <h1 class="title">游戏大厅</h1>
    <p class="subtitle">点击游戏标题进入详情页。</p>
    <el-alert v-if="err" type="warning" :title="err" show-icon style="margin-bottom: 16px" />
    <el-empty v-if="!err && games.length === 0" description="暂无游戏数据（请先启动 lgtbot_grpc_server）" />
    <el-row v-else :gutter="16">
      <el-col v-for="g in games" :key="g.name" :xs="24" :sm="12" :lg="8" class="card-col">
        <GameCard
          :game="g"
          :not-started-count="g.live_not_started"
          :started-count="g.live_in_progress"
        />
      </el-col>
    </el-row>
  </div>
</template>

<style scoped>
.home {
  width: 100%;
}
.title {
  margin: 0 0 8px;
  font-size: 1.5rem;
}
.subtitle {
  margin: 0 0 20px;
  color: var(--el-text-color-secondary);
  font-size: 14px;
}
.card-col {
  margin-bottom: 16px;
}
</style>
