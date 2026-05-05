<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue'
import { useAuthStore } from '../stores/auth'

const auth = useAuthStore()

const me = ref<{ display_name?: string; avatar_url?: string } | null>(null)

const displayName = computed(() => me.value?.display_name || auth.username || '—')

const timeRange = ref<'0' | '1' | '2'>('0')

interface WebGameLevelRow {
  game_name?: string
  count?: string | number
  total_level_score?: number | string
}

interface WebRecentMatchRow {
  game_name?: string
  finish_time?: string
  user_count?: string | number
  multiple?: string | number
  game_score?: string | number
  zero_sum_score?: string | number
  top_score?: string | number
  level_score?: number | string
  rank_score?: string | number
}

interface WebRecentHonorRow {
  id?: number
  description?: string
  time?: string
}

interface WebRecentAchievementRow {
  game_name?: string
  achievement_name?: string
  achievement_description?: string
  time?: string
}

interface ProfilePayload {
  total_zero_sum_score?: string | number
  total_top_score?: string | number
  match_count?: string | number
  birth_time?: string
  game_levels?: WebGameLevelRow[]
  recent_matches?: WebRecentMatchRow[]
  recent_honors?: WebRecentHonorRow[]
  recent_achievements?: WebRecentAchievementRow[]
}

const profile = ref<ProfilePayload | null>(null)
const profileErr = ref('')

function numish(v: unknown): string {
  if (v === undefined || v === null) return '—'
  return typeof v === 'number' ? String(v) : String(v)
}

async function loadMe() {
  if (!auth.token) return
  try {
    const res = await fetch('/api/me', {
      headers: { Authorization: `Bearer ${auth.token}` },
    })
    if (res.ok) me.value = await res.json()
  } catch {
    me.value = null
  }
}

async function loadProfile() {
  if (!auth.token) {
    profile.value = null
    return
  }
  profileErr.value = ''
  try {
    const res = await fetch(`/api/profile?time_range=${timeRange.value}`, {
      headers: { Authorization: `Bearer ${auth.token}` },
    })
    if (!res.ok) throw new Error(await res.text())
    profile.value = (await res.json()) as ProfilePayload
  } catch (e) {
    profileErr.value = e instanceof Error ? e.message : String(e)
    profile.value = null
  }
}

onMounted(() => {
  void loadMe()
  void loadProfile()
})
watch(() => auth.token, () => {
  void loadMe()
  void loadProfile()
})
watch(timeRange, () => {
  void loadProfile()
})

const levelRows = computed(() => profile.value?.game_levels ?? [])
const recentRows = computed(() => profile.value?.recent_matches ?? [])
const honorRows = computed(() => profile.value?.recent_honors ?? [])
const achieveRows = computed(() => profile.value?.recent_achievements ?? [])
</script>

<template>
  <div class="profile">
    <h1 class="title">个人信息</h1>

    <el-card class="block" shadow="never">
      <template #header>账号</template>
      <div class="account-row">
        <el-avatar v-if="me?.avatar_url" :size="56" :src="me.avatar_url" />
        <el-avatar v-else :size="56">{{ displayName.slice(0, 1).toUpperCase() }}</el-avatar>
        <div>
          <div class="line"><span class="label">用户名</span> {{ auth.username }}</div>
          <div class="line"><span class="label">显示名</span> {{ displayName }}</div>
        </div>
      </div>
    </el-card>

    <el-card class="block" shadow="never">
      <template #header>战绩时间范围</template>
      <el-radio-group v-model="timeRange" size="small">
        <el-radio-button label="0">本月</el-radio-button>
        <el-radio-button label="1">本年</el-radio-button>
        <el-radio-button label="2">总</el-radio-button>
      </el-radio-group>
      <div v-if="profile" class="summary-line">
        零和总分 {{ numish(profile.total_zero_sum_score) }} · 登顶分
        {{ numish(profile.total_top_score) }} · 对局
        {{ numish(profile.match_count) }}
        <span v-if="profile.birth_time" class="birth"> · 首次记录 {{ profile.birth_time }}</span>
      </div>
      <el-alert v-if="profileErr" type="warning" :title="profileErr" show-icon style="margin-top: 8px" />
    </el-card>

    <el-collapse class="block" model-value="levels">
      <el-collapse-item title="各游戏等级分与统计" name="levels">
        <el-table :data="levelRows" stripe size="small">
          <el-table-column prop="game_name" label="游戏" />
          <el-table-column label="局数" width="100">
            <template #default="scope">{{ numish(scope.row.count) }}</template>
          </el-table-column>
          <el-table-column label="累计等级分" width="120">
            <template #default="scope">{{ numish(scope.row.total_level_score) }}</template>
          </el-table-column>
        </el-table>
        <el-empty v-if="!levelRows.length && !profileErr" description="暂无等级分记录" />
      </el-collapse-item>
      <el-collapse-item title="近期战绩" name="recent">
        <el-table :data="recentRows" stripe size="small">
          <el-table-column type="index" label="#" width="56" />
          <el-table-column prop="game_name" label="游戏" />
          <el-table-column prop="finish_time" label="结束时间" width="170" />
          <el-table-column label="人数" width="72">
            <template #default="scope">{{ numish(scope.row.user_count) }}</template>
          </el-table-column>
          <el-table-column label="倍率" width="72">
            <template #default="scope">{{ numish(scope.row.multiple) }}</template>
          </el-table-column>
          <el-table-column label="等级分" width="96">
            <template #default="scope">{{ numish(scope.row.level_score) }}</template>
          </el-table-column>
        </el-table>
        <el-empty v-if="!recentRows.length && !profileErr" description="暂无对局记录" />
      </el-collapse-item>
      <el-collapse-item title="近期荣誉" name="honors">
        <el-table :data="honorRows" stripe size="small">
          <el-table-column prop="id" label="ID" width="72" />
          <el-table-column prop="description" label="内容" />
          <el-table-column prop="time" label="时间" width="170" />
        </el-table>
        <el-empty v-if="!honorRows.length && !profileErr" description="暂无荣誉记录" />
      </el-collapse-item>
      <el-collapse-item title="近期成就" name="achieve">
        <el-table :data="achieveRows" stripe size="small">
          <el-table-column prop="game_name" label="游戏" width="120" />
          <el-table-column prop="achievement_name" label="成就" />
          <el-table-column prop="achievement_description" label="说明" />
          <el-table-column prop="time" label="时间" width="170" />
        </el-table>
        <el-empty v-if="!achieveRows.length && !profileErr" description="暂无成就记录" />
      </el-collapse-item>
    </el-collapse>
  </div>
</template>

<style scoped>
.profile {
  max-width: 900px;
  margin: 0 auto;
}
.title {
  margin: 0 0 16px;
  font-size: 1.4rem;
}
.block {
  margin-bottom: 16px;
}
.account-row {
  display: flex;
  gap: 16px;
  align-items: center;
}
.line {
  margin: 4px 0;
  font-size: 14px;
}
.label {
  color: var(--el-text-color-secondary);
  margin-right: 8px;
}
.summary-line {
  margin-top: 12px;
  font-size: 14px;
  color: var(--el-text-color-regular);
}
.birth {
  color: var(--el-text-color-secondary);
  font-size: 13px;
}
</style>
