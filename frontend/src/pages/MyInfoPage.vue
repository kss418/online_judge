<template>
  <section class="page-grid single-column">
    <div class="my-info-layout">
      <article class="panel my-info-panel my-info-statistics-panel">
        <div class="panel-header">
          <div>
            <p class="panel-kicker">statistics</p>
            <h3>제출 통계</h3>
            <p class="my-info-copy">
              현재 계정의 제출 상태별 누적 횟수를 보여줍니다.
            </p>
          </div>

          <StatusBadge
            :label="statisticsStatusLabel"
            :tone="statisticsStatusTone"
          />
        </div>

        <div v-if="authState.isInitializing || isStatisticsLoading" class="empty-state">
          <p>제출 통계를 불러오는 중입니다.</p>
        </div>

        <div v-else-if="!isAuthenticated" class="empty-state">
          <p>로그인하면 제출 통계를 여기서 확인할 수 있습니다.</p>
        </div>

        <div v-else-if="statisticsErrorMessage" class="empty-state error-state">
          <p>{{ statisticsErrorMessage }}</p>
        </div>

        <div v-else class="my-info-summary">
          <div
            v-for="item in statisticsItems"
            :key="item.label"
            class="metric-row"
          >
            <SubmissionStatusBadge
              v-if="item.status"
              :status="item.status"
            />
            <span v-else class="metric-label">
              {{ item.label }}
            </span>
            <strong>{{ item.value }}</strong>
          </div>
        </div>
      </article>

      <div class="my-info-main-column">
        <article class="panel my-info-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">account</p>
              <h3>내 정보</h3>
              <p class="my-info-copy">
                현재 로그인한 계정의 기본 정보를 확인할 수 있습니다.
              </p>
            </div>

            <StatusBadge
              :label="sessionLabel"
              :tone="sessionTone"
            />
          </div>

          <div v-if="authState.isInitializing" class="empty-state">
            <p>내 정보를 확인하는 중입니다.</p>
          </div>

          <div v-else-if="!isAuthenticated" class="empty-state">
            <p>로그인하면 계정 기본 정보를 여기서 확인할 수 있습니다.</p>
          </div>

          <div v-else class="my-info-summary">
            <div class="metric-row">
              <span class="metric-label">ID</span>
              <strong>#{{ currentUser.id }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">닉네임</span>
              <strong>{{ currentUser.user_name }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">역할</span>
              <div class="my-info-role">
                <StatusBadge
                  :label="roleLabel"
                  :tone="roleTone"
                />
              </div>
            </div>
          </div>
        </article>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'

import { getMySubmissionStatistics } from '@/api/user'
import StatusBadge from '@/components/StatusBadge.vue'
import SubmissionStatusBadge from '@/components/submissions/SubmissionStatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const { authState, isAuthenticated, initializeAuth } = useAuth()
const submissionStatistics = ref(null)
const isStatisticsLoading = ref(true)
const statisticsErrorMessage = ref('')
let latestStatisticsRequestId = 0

const currentUser = computed(() => authState.currentUser ?? {
  id: 0,
  user_name: '',
  role_name: 'user',
  permission_level: 0
})

const sessionLabel = computed(() => {
  if (authState.isInitializing) {
    return 'Loading'
  }

  return isAuthenticated.value ? 'Signed In' : 'Guest'
})

const sessionTone = computed(() => {
  if (authState.isInitializing) {
    return 'neutral'
  }

  return isAuthenticated.value ? 'success' : 'neutral'
})

const roleLabel = computed(() => {
  if (currentUser.value.permission_level >= 2) {
    return 'SuperAdmin'
  }

  if (currentUser.value.permission_level >= 1) {
    return 'Admin'
  }

  return 'User'
})

const roleTone = computed(() => {
  if (currentUser.value.permission_level >= 2) {
    return 'danger'
  }

  if (currentUser.value.permission_level >= 1) {
    return 'warning'
  }

  return 'neutral'
})

const statisticsStatusLabel = computed(() => {
  if (authState.isInitializing || isStatisticsLoading.value) {
    return 'Loading'
  }

  if (!isAuthenticated.value) {
    return 'Guest'
  }

  return statisticsErrorMessage.value ? 'Error' : 'Loaded'
})

const statisticsStatusTone = computed(() => {
  if (authState.isInitializing || isStatisticsLoading.value) {
    return 'neutral'
  }

  if (statisticsErrorMessage.value) {
    return 'danger'
  }

  return isAuthenticated.value ? 'success' : 'neutral'
})

const statisticsItems = computed(() => {
  const statistics = submissionStatistics.value ?? {
    submission_count: 0,
    accepted_submission_count: 0,
    wrong_answer_submission_count: 0,
    time_limit_exceeded_submission_count: 0,
    memory_limit_exceeded_submission_count: 0,
    runtime_error_submission_count: 0,
    compile_error_submission_count: 0,
    output_exceeded_submission_count: 0,
    queued_submission_count: 0,
    judging_submission_count: 0,
    last_submission_at: null,
    last_accepted_at: null
  }

  return [
    {
      label: '전체 제출',
      value: statistics.submission_count
    },
    {
      label: '정답',
      value: statistics.accepted_submission_count,
      status: 'accepted'
    },
    {
      label: '오답',
      value: statistics.wrong_answer_submission_count,
      status: 'wrong_answer'
    },
    {
      label: '시간 초과',
      value: statistics.time_limit_exceeded_submission_count,
      status: 'time_limit_exceeded'
    },
    {
      label: '메모리 초과',
      value: statistics.memory_limit_exceeded_submission_count,
      status: 'memory_limit_exceeded'
    },
    {
      label: '런타임 에러',
      value: statistics.runtime_error_submission_count,
      status: 'runtime_error'
    },
    {
      label: '컴파일 에러',
      value: statistics.compile_error_submission_count,
      status: 'compile_error'
    },
    {
      label: '출력 초과',
      value: statistics.output_exceeded_submission_count,
      status: 'output_exceeded'
    },
    {
      label: '최근 제출',
      value: formatTimestamp(statistics.last_submission_at)
    },
    {
      label: '최근 정답',
      value: formatTimestamp(statistics.last_accepted_at)
    }
  ]
})

function normalizeCount(value){
  const numericValue = Number(value)
  return Number.isFinite(numericValue) ? numericValue : 0
}

function normalizeSubmissionStatistics(payload){
  return {
    user_id: Number(payload?.user_id ?? 0),
    submission_count: normalizeCount(payload?.submission_count),
    accepted_submission_count: normalizeCount(payload?.accepted_submission_count),
    wrong_answer_submission_count: normalizeCount(payload?.wrong_answer_submission_count),
    time_limit_exceeded_submission_count: normalizeCount(
      payload?.time_limit_exceeded_submission_count
    ),
    memory_limit_exceeded_submission_count: normalizeCount(
      payload?.memory_limit_exceeded_submission_count
    ),
    runtime_error_submission_count: normalizeCount(payload?.runtime_error_submission_count),
    compile_error_submission_count: normalizeCount(payload?.compile_error_submission_count),
    output_exceeded_submission_count: normalizeCount(payload?.output_exceeded_submission_count),
    queued_submission_count: normalizeCount(payload?.queued_submission_count),
    judging_submission_count: normalizeCount(payload?.judging_submission_count),
    last_submission_at: typeof payload?.last_submission_at === 'string'
      ? payload.last_submission_at
      : null,
    last_accepted_at: typeof payload?.last_accepted_at === 'string'
      ? payload.last_accepted_at
      : null
  }
}

function formatTimestamp(value){
  if (typeof value !== 'string' || !value.trim()) {
    return '-'
  }

  const trimmedValue = value.trim()
  const directMatch = trimmedValue.match(/^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2})/)
  if (directMatch) {
    return `${directMatch[1]} ${directMatch[2]}`
  }

  const parsedDate = new Date(trimmedValue)
  if (Number.isNaN(parsedDate.getTime())) {
    return trimmedValue
  }

  const year = String(parsedDate.getFullYear())
  const month = String(parsedDate.getMonth() + 1).padStart(2, '0')
  const day = String(parsedDate.getDate()).padStart(2, '0')
  const hours = String(parsedDate.getHours()).padStart(2, '0')
  const minutes = String(parsedDate.getMinutes()).padStart(2, '0')
  return `${year}-${month}-${day} ${hours}:${minutes}`
}

async function loadSubmissionStatistics(){
  const token = authState.token
  const currentUserId = Number(currentUser.value.id ?? 0)

  if (!token || currentUserId <= 0) {
    submissionStatistics.value = null
    statisticsErrorMessage.value = ''
    isStatisticsLoading.value = false
    return
  }

  const requestId = ++latestStatisticsRequestId
  isStatisticsLoading.value = true
  statisticsErrorMessage.value = ''

  try {
    const payload = await getMySubmissionStatistics(token)
    if (requestId !== latestStatisticsRequestId) {
      return
    }

    submissionStatistics.value = normalizeSubmissionStatistics(payload)
  } catch (error) {
    if (requestId !== latestStatisticsRequestId) {
      return
    }

    submissionStatistics.value = null
    statisticsErrorMessage.value = error instanceof Error
      ? error.message
      : '제출 통계를 불러오지 못했습니다.'
  } finally {
    if (requestId === latestStatisticsRequestId) {
      isStatisticsLoading.value = false
    }
  }
}

watch(
  () => [authState.initialized, authState.token, currentUser.value.id],
  ([initialized]) => {
    if (!initialized) {
      isStatisticsLoading.value = true
      return
    }

    if (!isAuthenticated.value) {
      latestStatisticsRequestId += 1
      submissionStatistics.value = null
      statisticsErrorMessage.value = ''
      isStatisticsLoading.value = false
      return
    }

    loadSubmissionStatistics()
  },
  {
    immediate: true
  }
)

onMounted(() => {
  initializeAuth()
})
</script>

<style scoped>
.my-info-layout {
  display: grid;
  grid-template-columns: minmax(364px, 468px) minmax(0, 1fr);
  gap: 1.25rem;
  align-items: start;
}

.my-info-main-column {
  display: grid;
  gap: 1.25rem;
  align-content: start;
}

.my-info-panel {
  display: grid;
  gap: 1.25rem;
}

.my-info-statistics-panel {
  align-self: start;
}

.my-info-copy {
  margin: 0.5rem 0 0;
  color: var(--text-muted);
  line-height: 1.6;
}

.my-info-summary {
  display: grid;
  gap: 0.85rem;
}

.my-info-role {
  display: inline-flex;
  align-items: center;
  justify-content: flex-end;
}

@media (max-width: 960px) {
  .my-info-layout {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
