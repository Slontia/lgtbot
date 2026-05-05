<script setup lang="ts">
import { computed } from 'vue'
import { RouterLink } from 'vue-router'

export interface GameRow {
  name: string
  /** 与游戏目录名一致，用于 URL */
  module_name?: string
  developer?: string
  description?: string
  min_players?: number
  max_players?: number
  live_not_started?: number
  live_in_progress?: number
  /** 通常为 `/api/game-modules/{module}/icon`，缺失时沿用首字母 */
  icon_url?: string
}

const props = defineProps<{
  game: GameRow
  /** 与同目录 icon.png 同源；未返回时沿用首字母 */
  notStartedCount?: number
  startedCount?: number
}>()

/** 路由段：优先 module_name；未返回时退回显示名，避免无链接 */
const slug = computed(() => {
  const m = props.game.module_name?.trim()
  if (m) return m
  return props.game.name
})

const gameLink = computed(() => ({
  name: 'game-detail' as const,
  params: { gameSlug: slug.value },
}))
</script>

<template>
  <el-card class="game-card" shadow="hover">
    <div class="card-inner">
      <div class="icon-wrap" aria-hidden="true">
        <img v-if="game.icon_url" class="game-icon-img" :src="game.icon_url" alt="" />
        <span v-else class="icon-fallback">{{ game.name.slice(0, 1) }}</span>
      </div>
      <div class="body">
        <RouterLink :to="gameLink" class="title-link">{{ game.name }}</RouterLink>
        <p class="desc">{{ game.description || '暂无说明' }}</p>
        <div class="stats">
          当前桌数：未开始 {{ notStartedCount ?? 0 }} / 进行中 {{ startedCount ?? 0 }}
        </div>
      </div>
    </div>
  </el-card>
</template>

<style scoped>
.game-card {
  height: 100%;
}
.card-inner {
  display: flex;
  gap: 14px;
  align-items: flex-start;
}
.icon-wrap {
  flex: 0 0 72px;
  width: 72px;
  height: 72px;
  border-radius: 10px;
  overflow: hidden;
}
.game-icon-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  display: block;
}
.icon-fallback {
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, var(--el-color-primary-light-7), var(--el-fill-color));
  font-size: 28px;
  font-weight: 600;
  color: var(--el-color-primary);
}
.body {
  min-width: 0;
  flex: 1;
}
.title-link {
  font-size: 1.1rem;
  font-weight: 600;
  color: var(--el-text-color-primary);
  text-decoration: none;
  transition: color 0.15s ease;
}
.title-link:hover {
  color: var(--el-color-primary);
}
.desc {
  margin: 8px 0 10px;
  font-size: 13px;
  color: var(--el-text-color-secondary);
  line-height: 1.45;
  display: -webkit-box;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
  overflow: hidden;
}
.stats {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin-bottom: 0;
}
</style>
