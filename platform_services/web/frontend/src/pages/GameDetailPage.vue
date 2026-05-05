<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { Calendar, Setting, UserFilled } from '@element-plus/icons-vue'
import { ElMessage } from 'element-plus'
import { RouterLink, useRoute, useRouter } from 'vue-router'
import { marked } from 'marked'
import type { GameRow } from '../components/GameCard.vue'
import { useAuthStore } from '../stores/auth'

const route = useRoute()
const router = useRouter()
const auth = useAuthStore()

const games = ref<GameRow[]>([])
const loadErr = ref('')
const soloLoading = ref(false)

const gameSlug = computed(() => (route.params.gameSlug as string) || '')

const game = computed(() =>
  games.value.find(
    (g) =>
      (g.module_name && g.module_name === gameSlug.value) ||
      g.name === gameSlug.value ||
      decodeURIComponent(gameSlug.value) === g.name,
  ),
)

interface WebMatchBrief {
  match_id?: string | number
  state?: number
  room_label?: string
  config_text?: string
  room_created_unix?: string | number
  game_started_unix?: string | number
  players?: { platform_user_id?: string; display_name?: string; avatar_url?: string }[]
}

const matchesTotal = ref(0)
const matchRows = ref<WebMatchBrief[]>([])
const matchesLoadErr = ref('')

const ruleMarkdown = ref('')
const ruleLoadErr = ref('')
const ruleHtml = computed(() =>
  ruleMarkdown.value ? (marked.parse(ruleMarkdown.value) as string) : '',
)

interface AchItem {
  name?: string
  description?: string
  achieved_user_count?: string | number
}

const achievements = ref<AchItem[]>([])
const achLoadErr = ref('')

interface RankEntry {
  platform_user_id?: string
  display_name?: string
  avatar_url?: string
  score_value?: number | string
  match_count_value?: string | number
}

interface RankingsPack {
  level_score?: RankEntry[]
  weighted_level_score?: RankEntry[]
  match_count?: RankEntry[]
}

const rankings = ref<RankingsPack | null>(null)
const rankLoadErr = ref('')

const pageSize = 16
const currentPage = ref(1)

const timeTab = ref('month')
const rankMetricTab = ref('level')

function numish(v: unknown): number {
  if (typeof v === 'number' && !Number.isNaN(v)) return v
  if (typeof v === 'string' && v !== '') return Number(v)
  return 0
}

function fmtUnix(sec: unknown): string {
  const n = numish(sec)
  if (n <= 0) return '尚未开始'
  return new Date(n * 1000).toLocaleString()
}

function stateLabelFromApi(code: unknown): string {
  const n = typeof code === 'number' ? code : numish(code)
  if (n <= 0) return '—'
  const c = String.fromCharCode(n)
  if (c === 'N') return '未开始'
  if (c === 'S') return '进行中'
  if (c === 'O') return '已结束'
  return c || '—'
}

async function loadGames() {
  loadErr.value = ''
  try {
    const res = await fetch('/api/games')
    if (!res.ok) throw new Error(await res.text())
    const data = (await res.json()) as { games?: GameRow[] }
    games.value = data.games ?? []
  } catch (e) {
    loadErr.value = e instanceof Error ? e.message : String(e)
  }
}

async function loadMatches() {
  const g = game.value
  if (!g) return
  matchesLoadErr.value = ''
  try {
    const off = (currentPage.value - 1) * pageSize
    const enc = encodeURIComponent(g.name)
    const res = await fetch(`/api/games/${enc}/matches?offset=${off}&limit=${pageSize}`)
    if (!res.ok) throw new Error(await res.text())
    const data = (await res.json()) as { matches?: WebMatchBrief[]; total?: number | string }
    matchRows.value = data.matches ?? []
    matchesTotal.value = numish(data.total)
  } catch (e) {
    matchesLoadErr.value = e instanceof Error ? e.message : String(e)
    matchRows.value = []
    matchesTotal.value = 0
  }
}

async function loadRule() {
  const g = game.value
  if (!g) return
  ruleLoadErr.value = ''
  try {
    const enc = encodeURIComponent(g.name)
    const res = await fetch(`/api/games/${enc}/rule`)
    if (!res.ok) throw new Error(await res.text())
    const data = (await res.json()) as { pure_rule_markdown?: string }
    ruleMarkdown.value = data.pure_rule_markdown ?? ''
  } catch (e) {
    ruleLoadErr.value = e instanceof Error ? e.message : String(e)
  }
}

async function loadAchievements() {
  const g = game.value
  if (!g) return
  achLoadErr.value = ''
  try {
    const enc = encodeURIComponent(g.name)
    const headers: Record<string, string> = {}
    if (auth.token) headers.Authorization = `Bearer ${auth.token}`
    const res = await fetch(`/api/games/${enc}/achievements`, { headers })
    if (!res.ok) throw new Error(await res.text())
    const data = (await res.json()) as { items?: AchItem[] }
    achievements.value = data.items ?? []
  } catch (e) {
    achLoadErr.value = e instanceof Error ? e.message : String(e)
  }
}

function timeRangeQuery(): string {
  if (timeTab.value === 'year') return '1'
  if (timeTab.value === 'all') return '2'
  return '0'
}

async function loadRankings() {
  const g = game.value
  if (!g) return
  rankLoadErr.value = ''
  try {
    const enc = encodeURIComponent(g.name)
    const tr = timeRangeQuery()
    const res = await fetch(`/api/games/${enc}/rankings?time_range=${tr}&top_n=10`)
    if (!res.ok) throw new Error(await res.text())
    rankings.value = (await res.json()) as RankingsPack
  } catch (e) {
    rankLoadErr.value = e instanceof Error ? e.message : String(e)
    rankings.value = null
  }
}

const rankTableRows = computed(() => {
  const pack = rankings.value
  if (!pack) return []
  const metric = rankMetricTab.value
  const list =
    metric === 'level'
      ? pack.level_score
      : metric === 'weighted'
        ? pack.weighted_level_score
        : pack.match_count
  if (!list?.length) return []
  return list.map((row, i) => {
    const score =
      metric === 'matches'
        ? String(numish(row.match_count_value))
        : typeof row.score_value === 'number'
          ? row.score_value.toFixed(3)
          : String(row.score_value ?? '—')
    return {
      rank: i + 1,
      user: row.display_name || row.platform_user_id || '—',
      score,
      avatar_url: row.avatar_url,
    }
  })
})

const scoreColumnLabel = computed(() => (rankMetricTab.value === 'matches' ? '局数' : '分数'))

interface TablePlayer {
  id: string
  label: string
  avatar_url?: string
}

const tableRows = computed(() =>
  matchRows.value.map((m) => {
    const players: TablePlayer[] =
      m.players?.map((p) => ({
        id: p.platform_user_id ?? '',
        label: p.display_name ?? p.platform_user_id ?? '—',
        avatar_url: p.avatar_url,
      })) ?? []
    return {
      matchId: numish(m.match_id),
      state: m.state,
      roomLabel: m.room_label ?? '',
      config: m.config_text ?? '',
      createdAt: fmtUnix(m.room_created_unix),
      startedAt: fmtUnix(m.game_started_unix),
      stateLabel: stateLabelFromApi(m.state),
      players,
      pv: visiblePlayers(players),
    }
  }),
)

function firstChar(name: string) {
  return name.slice(0, 1).toUpperCase()
}

function visiblePlayers(players: TablePlayer[]) {
  if (players.length <= 8) {
    return { head: players, ellipsis: false, restCount: 0 }
  }
  return { head: players.slice(0, 7), ellipsis: true, restCount: players.length - 7 }
}

async function soloPlay() {
  const g = game.value
  if (!g) return
  if (!auth.token) {
    ElMessage.warning('请先登录后再试玩')
    await router.push({ name: 'login', query: { redirect: route.fullPath } })
    return
  }
  soloLoading.value = true
  try {
    const res = await fetch('/api/game/start', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        Authorization: `Bearer ${auth.token}`,
      },
      body: JSON.stringify({ game_name: g.name, solo: true }),
    })
    const txt = await res.text()
    if (!res.ok) throw new Error(txt)
    ElMessage.success(`已向 bot 发送单机试玩：${g.name}`)
    await router.push({ name: 'game' })
  } catch (e) {
    ElMessage.error(e instanceof Error ? e.message : String(e))
  } finally {
    soloLoading.value = false
  }
}

function createRoom() {
  ElMessage.info('创建房间功能尚未接入，敬请期待')
}

onMounted(() => {
  void loadGames()
})

watch(
  game,
  () => {
    if (!game.value) return
    currentPage.value = 1
    void (async () => {
      await Promise.all([loadRule(), loadAchievements(), loadRankings(), loadMatches()])
    })()
  },
  { immediate: true },
)

watch(currentPage, () => {
  if (game.value) void loadMatches()
})

watch(timeTab, () => {
  if (game.value) void loadRankings()
})
</script>

<template>
  <div v-if="loadErr" class="page">
    <el-alert type="error" :title="loadErr" show-icon />
  </div>
  <div v-else-if="!game" class="page">
    <el-empty :description="`未找到游戏：${gameSlug}`">
      <RouterLink to="/">返回首页</RouterLink>
    </el-empty>
  </div>
  <div v-else class="page game-detail">
    <!-- 上：游戏信息 -->
    <section class="section hero">
      <div class="hero-left">
        <div class="hero-icon" aria-hidden="true">
          <img v-if="game.icon_url" class="hero-icon-img" :src="game.icon_url" alt="" />
          <template v-else>{{ firstChar(game.name) }}</template>
        </div>
      </div>
      <div class="hero-text">
        <h1 class="hero-title">{{ game.name }}</h1>
        <p class="hero-desc">{{ game.description || '暂无说明' }}</p>
        <div class="meta">
          <el-tag v-if="game.max_players != null" size="small" type="info">
            最多 {{ game.max_players }} 人
          </el-tag>
          <span v-if="game.developer" class="dev">开发者：{{ game.developer }}</span>
        </div>
      </div>
      <div class="hero-actions">
        <el-button type="primary" class="hero-action-btn" :loading="soloLoading" @click="soloPlay">
          单机试玩
        </el-button>
        <el-button class="hero-action-btn" @click="createRoom">创建房间</el-button>
      </div>
    </section>

    <!-- 中：当前桌 -->
    <section class="section">
      <h2 class="section-title">当前比赛</h2>
      <el-alert
        v-if="matchesLoadErr"
        type="warning"
        :title="matchesLoadErr"
        show-icon
        style="margin-bottom: 12px"
      />
      <p class="hint">与 bot「#赛事列表」同源；每页 {{ pageSize }} 条。</p>
      <div class="tables-grid">
        <el-popover
          v-for="row in tableRows"
          :key="row.matchId"
          trigger="hover"
          placement="right-start"
          :width="400"
          :show-after="120"
          popper-class="game-room-popover"
        >
          <template #default>
            <div class="room-pop">
              <header class="room-pop-head">
                <div class="room-pop-head-main">
                  <span class="room-pop-label">比赛</span>
                  <h4 class="room-pop-title">
                    {{ game!.name }}<span class="room-pop-hash">#{{ row.matchId }}</span>
                  </h4>
                  <div class="room-state-line">状态：{{ row.stateLabel }} · 房间 {{ row.roomLabel }}</div>
                </div>
              </header>
              <div class="room-pop-body">
                <section class="room-section">
                  <div class="room-section-head">
                    <el-icon class="room-ico" :size="15"><Calendar /></el-icon>
                    <span>时间</span>
                  </div>
                  <div class="room-kv">
                    <div class="room-kv-row">
                      <span class="room-k">创建</span>
                      <span class="room-v">{{ row.createdAt }}</span>
                    </div>
                    <div class="room-kv-row">
                      <span class="room-k">开局</span>
                      <span class="room-v" :class="{ 'room-v-muted': row.startedAt === '尚未开始' }">
                        {{ row.startedAt }}
                      </span>
                    </div>
                  </div>
                </section>
                <section class="room-section room-section-players">
                  <div class="room-section-head">
                    <el-icon class="room-ico" :size="15"><UserFilled /></el-icon>
                    <span>玩家</span>
                    <span class="room-badge">{{ row.players.length }} 人</span>
                  </div>
                  <div class="popover-avatars">
                    <el-tooltip
                      v-for="p in row.players"
                      :key="p.id"
                      placement="top"
                      :show-after="200"
                    >
                      <template #content>
                        <div class="tip-player">
                          <div class="tip-name">{{ p.label }}</div>
                          <div class="tip-detail">{{ p.id }}</div>
                        </div>
                      </template>
                      <el-avatar
                        :size="34"
                        class="pop-av"
                        :src="p.avatar_url || undefined"
                      >
                        {{ firstChar(p.label) }}
                      </el-avatar>
                    </el-tooltip>
                  </div>
                </section>
                <section class="room-section room-section-config">
                  <div class="room-section-head">
                    <el-icon class="room-ico" :size="15"><Setting /></el-icon>
                    <span>配置</span>
                  </div>
                  <pre class="config-pre">{{ row.config }}</pre>
                </section>
              </div>
            </div>
          </template>
          <template #reference>
            <div class="table-card">
              <div class="tc-top">
                <div class="thumb">
                  <img v-if="game!.icon_url" class="thumb-img" :src="game!.icon_url" alt="" />
                  <template v-else>{{ firstChar(game!.name) }}</template>
                </div>
                <div class="title-block">
                  <span class="gid">{{ game!.name }}#{{ row.matchId }}</span>
                  <span class="gid-sub">{{ row.stateLabel }}</span>
                </div>
              </div>
              <div class="tc-avatars">
                <el-avatar
                  v-for="p in row.pv.head"
                  :key="p.id"
                  :size="26"
                  class="p-av"
                  :src="p.avatar_url || undefined"
                >
                  {{ firstChar(p.label) }}
                </el-avatar>
                <el-tooltip
                  v-if="row.pv.ellipsis"
                  content="其余玩家请在悬停浮层中查看头像"
                  placement="top"
                >
                  <el-avatar :size="26" class="p-av ell">···</el-avatar>
                </el-tooltip>
              </div>
            </div>
          </template>
        </el-popover>
      </div>
      <el-pagination
        v-model:current-page="currentPage"
        class="pager"
        background
        layout="prev, pager, next, ->, total"
        :total="matchesTotal"
        :page-size="pageSize"
        hide-on-single-page
      />
    </section>

    <!-- 下：规则 / 成就 / 排行 -->
    <section class="section triple">
      <div class="col col-pane rules">
        <h3 class="block-title">游戏规则</h3>
        <el-alert v-if="ruleLoadErr" type="warning" :title="ruleLoadErr" show-icon style="margin-bottom: 8px" />
        <el-scrollbar class="pane-scroll">
          <div v-if="ruleHtml" class="rule-md" v-html="ruleHtml" />
          <span v-else class="rule-placeholder">暂无规则文本</span>
        </el-scrollbar>
      </div>
      <div class="col col-pane achieve">
        <h3 class="block-title">成就</h3>
        <el-alert v-if="achLoadErr" type="warning" :title="achLoadErr" show-icon style="margin-bottom: 8px" />
        <el-scrollbar class="pane-scroll">
          <el-empty v-if="!achievements.length && !achLoadErr" description="该游戏未声明成就" :image-size="64" />
          <ul v-else class="ach-list">
            <li v-for="(a, i) in achievements" :key="i" class="ach-item">
              <div class="ach-name">{{ a.name }}</div>
              <div class="ach-desc">{{ a.description }}</div>
              <div class="ach-meta">达成人数：{{ a.achieved_user_count ?? 0 }}</div>
            </li>
          </ul>
        </el-scrollbar>
      </div>
      <div class="col col-pane ranks">
        <h3 class="block-title">排名（前 10 名）</h3>
        <el-alert v-if="rankLoadErr" type="warning" :title="rankLoadErr" show-icon style="margin-bottom: 8px" />
        <el-tabs v-model="timeTab" class="time-tabs">
          <el-tab-pane label="月排名" name="month" />
          <el-tab-pane label="年排名" name="year" />
          <el-tab-pane label="总排名" name="all" />
        </el-tabs>
        <el-tabs v-model="rankMetricTab" type="card" size="small" class="metric-tabs">
          <el-tab-pane label="等级分" name="level" />
          <el-tab-pane label="等级分×局数" name="weighted" />
          <el-tab-pane label="游戏局数" name="matches" />
        </el-tabs>
        <el-scrollbar class="pane-scroll pane-scroll-table">
          <el-table :data="rankTableRows" stripe size="small" class="rank-table" empty-text="暂无排行数据">
            <el-table-column prop="rank" label="排名" width="64" />
            <el-table-column prop="user" label="用户">
              <template #default="scope">
                <div class="rank-user">
                  <el-avatar
                    v-if="scope.row.avatar_url"
                    :size="28"
                    :src="scope.row.avatar_url"
                  />
                  <span>{{ scope.row.user }}</span>
                </div>
              </template>
            </el-table-column>
            <el-table-column prop="score" :label="scoreColumnLabel" width="104" />
          </el-table>
        </el-scrollbar>
        <p class="hint inner-hint">与 bot「#排行」同一数据源；切换页签可切换时间范围与指标。</p>
      </div>
    </section>

    <div class="footer-links">
      <RouterLink to="/game">Bot 私聊指令控制台 (WS)</RouterLink>
    </div>
  </div>
</template>

<style scoped>
.page {
  width: 100%;
}
.game-detail {
  display: flex;
  flex-direction: column;
  gap: 28px;
}
.section-title {
  margin: 0 0 8px;
  font-size: 1.15rem;
}
.block-title {
  margin: 0 0 10px;
  flex-shrink: 0;
  font-size: 1rem;
}
.hint {
  font-size: 12px;
  color: var(--el-text-color-secondary);
  margin: 0 0 12px;
}
.inner-hint {
  margin: 8px 0 0;
  flex-shrink: 0;
}
.hero {
  display: flex;
  gap: 24px;
  align-items: flex-start;
  padding: 16px;
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 12px;
  background: var(--el-fill-color-blank);
}
.hero-left {
  flex: 0 0 auto;
}
.hero-icon {
  width: 72px;
  height: 72px;
  border-radius: 12px;
  background: linear-gradient(135deg, var(--el-color-primary-light-7), var(--el-fill-color));
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 32px;
  font-weight: 700;
  color: var(--el-color-primary);
  overflow: hidden;
}
.hero-icon-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  display: block;
}
.hero-actions {
  flex: 0 0 auto;
  display: flex;
  flex-direction: row;
  flex-wrap: wrap;
  gap: 10px;
  align-self: flex-start;
  justify-content: flex-end;
}
.hero-action-btn {
  width: 132px;
}
.hero-text {
  flex: 1;
  min-width: 0;
}
.hero-title {
  margin: 0 0 8px;
  font-size: 1.35rem;
}
.hero-desc {
  margin: 0 0 12px;
  color: var(--el-text-color-regular);
  line-height: 1.5;
}
.meta {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 8px 12px;
}
.dev {
  font-size: 13px;
  color: var(--el-text-color-secondary);
}
.tables-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 10px;
  margin-bottom: 12px;
}
@media (max-width: 1100px) {
  .tables-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }
}
@media (max-width: 840px) {
  .tables-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
@media (max-width: 480px) {
  .tables-grid {
    grid-template-columns: 1fr;
  }
}
.table-card {
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 10px;
  padding: 8px 10px;
  background: var(--el-bg-color);
  cursor: default;
  min-width: 0;
}
.tc-top {
  display: flex;
  gap: 8px;
  align-items: center;
  margin-bottom: 6px;
}
.thumb {
  flex: 0 0 36px;
  height: 36px;
  width: 36px;
  border-radius: 8px;
  background: var(--el-fill-color);
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 600;
  color: var(--el-color-primary);
  font-size: 15px;
  overflow: hidden;
}
.thumb-img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  display: block;
}
.title-block {
  min-width: 0;
}
.gid {
  font-weight: 600;
  font-size: 12px;
  line-height: 1.3;
  display: block;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.gid-sub {
  display: block;
  font-size: 11px;
  color: var(--el-text-color-secondary);
  margin-top: 2px;
}
.tc-avatars {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  align-items: center;
  min-height: 26px;
}
.p-av {
  cursor: default;
  font-size: 11px;
}
.p-av.ell {
  font-weight: 700;
  background: var(--el-fill-color-dark);
}
.pager {
  justify-content: flex-end;
}
.triple {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 16px;
  align-items: stretch;
}
@media (max-width: 960px) {
  .triple {
    grid-template-columns: 1fr;
  }
}
.col-pane {
  /* 规则 / 成就 / 排名同高；需容纳双层 Tab + 约 10 行表格 + 底部说明 */
  height: 580px;
  display: flex;
  flex-direction: column;
  box-sizing: border-box;
  overflow: hidden;
}
.col {
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 10px;
  padding: 12px;
  background: var(--el-bg-color);
}
.pane-scroll {
  flex: 1;
  min-height: 0;
}
.pane-scroll :deep(.el-scrollbar__wrap) {
  overflow-x: hidden;
}
.pane-scroll :deep(.el-scrollbar__view) {
  padding-right: 12px;
}
.pane-scroll-table {
  padding-right: 4px;
}
.placeholder {
  font-size: 13px;
  color: var(--el-text-color-secondary);
  line-height: 1.5;
  margin: 0 0 10px;
  white-space: pre-wrap;
}
.rule-placeholder {
  font-size: 13px;
  color: var(--el-text-color-secondary);
}
.rule-md {
  font-size: 14px;
  line-height: 1.7;
  color: var(--el-text-color-regular);
}
.rule-md :deep(h1),
.rule-md :deep(h2),
.rule-md :deep(h3),
.rule-md :deep(h4) {
  margin: 0.9em 0 0.4em;
  font-weight: 600;
  line-height: 1.3;
}
.rule-md :deep(h1) { font-size: 1.5em; border-bottom: 1px solid var(--el-border-color-lighter); padding-bottom: 0.2em; }
.rule-md :deep(h2) { font-size: 1.25em; }
.rule-md :deep(h3) { font-size: 1.1em; }
.rule-md :deep(p) { margin: 0.4em 0 0.7em; }
.rule-md :deep(ul),
.rule-md :deep(ol) { padding-left: 1.6em; margin: 0.3em 0 0.7em; }
.rule-md :deep(li) { margin: 0.2em 0; }
.rule-md :deep(code) {
  font-family: 'SFMono-Regular', Consolas, Menlo, monospace;
  font-size: 0.85em;
  background: var(--el-fill-color-light);
  padding: 0.1em 0.35em;
  border-radius: 3px;
}
.rule-md :deep(pre) {
  background: var(--el-fill-color-light);
  border: 1px solid var(--el-border-color-lighter);
  border-radius: 6px;
  padding: 10px 14px;
  overflow-x: auto;
  margin: 0.5em 0 0.9em;
}
.rule-md :deep(pre) code {
  background: none;
  padding: 0;
}
.rule-md :deep(blockquote) {
  border-left: 3px solid var(--el-border-color);
  color: var(--el-text-color-secondary);
  margin: 0.4em 0;
  padding: 0.2em 0.8em;
}
.rule-md :deep(table) {
  border-collapse: collapse;
  margin: 0.5em 0 0.9em;
}
.rule-md :deep(th),
.rule-md :deep(td) {
  border: 1px solid var(--el-border-color);
  padding: 5px 10px;
}
.rule-md :deep(th) { background: var(--el-fill-color-light); font-weight: 600; }
.rule-md :deep(hr) {
  border: none;
  border-top: 1px solid var(--el-border-color-lighter);
  margin: 1em 0;
}
.rule-md :deep(strong) { font-weight: 600; }
.rule-md :deep(a) { color: var(--el-color-primary); }
.ach-list {
  margin: 0;
  padding: 0 0 0 18px;
}
.ach-item {
  margin-bottom: 12px;
}
.ach-name {
  font-weight: 600;
  font-size: 14px;
}
.ach-desc {
  font-size: 13px;
  color: var(--el-text-color-regular);
  margin: 4px 0;
}
.ach-meta {
  font-size: 12px;
  color: var(--el-text-color-secondary);
}
.rank-user {
  display: flex;
  align-items: center;
  gap: 8px;
}
.rank-table {
  width: 100%;
}
.time-tabs :deep(.el-tabs__header) {
  margin-bottom: 6px;
}
.metric-tabs {
  margin-bottom: 6px;
}
.ranks {
  gap: 0;
}
.footer-links {
  font-size: 13px;
}
</style>
<style>
/* 比赛浮层挂载到 body，使用 popper-class 单独样式 */
.game-room-popover.el-popper {
  padding: 0;
  border-radius: 12px;
  overflow: hidden;
  border: 1px solid var(--el-border-color-light);
  box-shadow: var(--el-box-shadow);
}

.game-room-popover .room-pop-head {
  padding: 12px 14px;
  background: linear-gradient(
    125deg,
    var(--el-color-primary-light-9) 0%,
    var(--el-fill-color-blank) 48%,
    var(--el-bg-color) 100%
  );
  border-bottom: 1px solid var(--el-border-color-lighter);
}

.game-room-popover .room-pop-label {
  display: block;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.06em;
  color: var(--el-color-primary);
  margin-bottom: 4px;
}

.game-room-popover .room-pop-title {
  margin: 0;
  font-size: 15px;
  font-weight: 700;
  line-height: 1.35;
  color: var(--el-text-color-primary);
  word-break: break-word;
}

.game-room-popover .room-state-line {
  margin-top: 6px;
  font-size: 11px;
  color: var(--el-text-color-secondary);
}

.game-room-popover .room-pop-hash {
  margin-left: 6px;
  font-weight: 700;
  color: var(--el-color-primary);
  font-variant-numeric: tabular-nums;
}

.game-room-popover .room-pop-body {
  padding: 10px 12px 12px;
  max-height: 400px;
  overflow-y: auto;
  background: var(--el-bg-color);
}

.game-room-popover .room-section {
  margin-bottom: 10px;
  padding: 10px 12px;
  border-radius: 10px;
  background: var(--el-fill-color-light);
  border: 1px solid var(--el-border-color-extra-light);
}

.game-room-popover .room-section:last-child {
  margin-bottom: 0;
}

.game-room-popover .room-section-head {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 8px;
  font-size: 12px;
  font-weight: 600;
  color: var(--el-text-color-regular);
}

.game-room-popover .room-ico {
  color: var(--el-color-primary);
}

.game-room-popover .room-badge {
  margin-left: auto;
  font-size: 11px;
  font-weight: 600;
  color: var(--el-color-primary);
  background: var(--el-color-primary-light-9);
  padding: 2px 8px;
  border-radius: 999px;
}

.game-room-popover .room-kv {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.game-room-popover .room-kv-row {
  display: grid;
  grid-template-columns: 40px 1fr;
  gap: 8px;
  align-items: baseline;
  font-size: 12px;
  line-height: 1.45;
}

.game-room-popover .room-k {
  color: var(--el-text-color-secondary);
  font-size: 11px;
}

.game-room-popover .room-v {
  font-variant-numeric: tabular-nums;
  word-break: break-word;
  color: var(--el-text-color-primary);
}

.game-room-popover .room-v-muted {
  color: var(--el-text-color-secondary);
  font-style: italic;
}

.game-room-popover .room-section-players .popover-avatars {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.game-room-popover .room-section-players .pop-av {
  cursor: default;
  flex-shrink: 0;
  box-shadow: 0 0 0 1px var(--el-border-color-light);
  transition:
    transform 0.15s ease,
    box-shadow 0.15s ease;
}

.game-room-popover .room-section-players .pop-av:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.game-room-popover .room-section-config .config-pre {
  margin: 0;
  padding: 10px 12px;
  white-space: pre-wrap;
  font-size: 11px;
  line-height: 1.55;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
  color: var(--el-text-color-regular);
  background: var(--el-fill-color-blank);
  border-radius: 8px;
  border: 1px solid var(--el-border-color-lighter);
  border-left: 3px solid var(--el-color-primary-light-3);
}

.game-room-popover .tip-player {
  max-width: 260px;
}

.game-room-popover .tip-name {
  font-weight: 600;
  margin-bottom: 6px;
  font-size: 13px;
}

.game-room-popover .tip-detail {
  font-size: 12px;
  line-height: 1.45;
  color: var(--el-text-color-regular);
}
</style>
