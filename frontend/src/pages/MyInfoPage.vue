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
              <span class="metric-label">권한</span>
              <div class="my-info-role">
                <StatusBadge
                  :label="roleLabel"
                  :tone="roleTone"
                />
              </div>
            </div>
          </div>
        </article>

        <article class="panel my-info-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">solved</p>
              <h3>푼 문제 목록</h3>
              <p class="my-info-copy">
                현재 계정이 정답 처리한 문제를 바로 확인할 수 있습니다.
              </p>
            </div>

            <StatusBadge
              :label="solvedProblemsStatusLabel"
              :tone="solvedProblemsStatusTone"
            />
          </div>

          <div v-if="authState.isInitializing || isSolvedProblemsLoading" class="empty-state">
            <p>푼 문제 목록을 불러오는 중입니다.</p>
          </div>

          <div v-else-if="!isAuthenticated" class="empty-state">
            <p>로그인하면 푼 문제 목록을 여기서 확인할 수 있습니다.</p>
          </div>

          <div v-else-if="solvedProblemsErrorMessage" class="empty-state error-state">
            <p>{{ solvedProblemsErrorMessage }}</p>
          </div>

          <div v-else-if="solvedProblems.length === 0" class="empty-state">
            <p>아직 푼 문제가 없습니다.</p>
          </div>

          <div v-else class="solved-problem-grid">
            <RouterLink
              v-for="problem in solvedProblems"
              :key="problem.problem_id"
              class="solved-problem-chip"
              :to="{ name: 'problem-detail', params: { problemId: problem.problem_id } }"
            >
              <strong class="solved-problem-chip__id">
                #{{ problem.problem_id }}
              </strong>
              <div class="solved-problem-chip__meta">
                <span class="solved-problem-chip__label">정답</span>
                <strong>{{ problem.accepted_count }}</strong>
              </div>
            </RouterLink>
          </div>
        </article>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'

import { getMySolvedProblems, getMySubmissionStatistics } from '@/api/user'
import StatusBadge from '@/components/StatusBadge.vue'
import SubmissionStatusBadge from '@/components/submissions/SubmissionStatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const { authState, isAuthenticated, initializeAuth } = useAuth()
const submissionStatistics = ref(null)
const solvedProblems = ref([])
const isStatisticsLoading = ref(true)
const isSolvedProblemsLoading = ref(true)
const statisticsErrorMessage = ref('')
const solvedProblemsErrorMessage = ref('')
let latestStatisticsRequestId = 0
let latestSolvedProblemsRequestId = 0

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

const solvedProblemsStatusLabel = computed(() => {
  if (authState.isInitializing || isSolvedProblemsLoading.value) {
    return 'Loading'
  }

  if (!isAuthenticated.value) {
    return 'Guest'
  }

  if (solvedProblemsErrorMessage.value) {
    return 'Error'
  }

  return `${solvedProblems.value.length} Solved`
})

const solvedProblemsStatusTone = computed(() => {
  if (authState.isInitializing || isSolvedProblemsLoading.value) {
    return 'neutral'
  }

  if (solvedProblemsErrorMessage.value) {
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

function normalizeSolvedProblems(payload){
  const solvedProblemValues = Array.isArray(payload?.solved_problems)
    ? payload.solved_problems
    : []

  return solvedProblemValues
    .map((problem) => ({
      problem_id: Number(problem?.problem_id ?? 0),
      accepted_count: normalizeCount(problem?.accepted_count)
    }))
    .filter((problem) => problem.problem_id > 0)
    .sort((leftProblem, rightProblem) => leftProblem.problem_id - rightProblem.problem_id)
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

async function loadSolvedProblems(){
  const token = authState.token
  const currentUserId = Number(currentUser.value.id ?? 0)

  if (!token || currentUserId <= 0) {
    solvedProblems.value = []
    solvedProblemsErrorMessage.value = ''
    isSolvedProblemsLoading.value = false
    return
  }

  const requestId = ++latestSolvedProblemsRequestId
  isSolvedProblemsLoading.value = true
  solvedProblemsErrorMessage.value = ''

  try {
    const payload = await getMySolvedProblems(token)
    if (requestId !== latestSolvedProblemsRequestId) {
      return
    }

    solvedProblems.value = normalizeSolvedProblems(payload)
  } catch (error) {
    if (requestId !== latestSolvedProblemsRequestId) {
      return
    }

    solvedProblems.value = []
    solvedProblemsErrorMessage.value = error instanceof Error
      ? error.message
      : '푼 문제 목록을 불러오지 못했습니다.'
  } finally {
    if (requestId === latestSolvedProblemsRequestId) {
      isSolvedProblemsLoading.value = false
    }
  }
}

watch(
  () => [authState.initialized, authState.token, currentUser.value.id],
  ([initialized]) => {
    if (!initialized) {
      isStatisticsLoading.value = true
      isSolvedProblemsLoading.value = true
      return
    }

    if (!isAuthenticated.value) {
      latestStatisticsRequestId += 1
      latestSolvedProblemsRequestId += 1
      submissionStatistics.value = null
      solvedProblems.value = []
      statisticsErrorMessage.value = ''
      solvedProblemsErrorMessage.value = ''
      isStatisticsLoading.value = false
      isSolvedProblemsLoading.value = false
      return
    }

    loadSubmissionStatistics()
    loadSolvedProblems()
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

.solved-problem-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(116px, 1fr));
  gap: 0.8rem;
  max-height: 28rem;
  overflow: auto;
  padding-right: 0.2rem;
}

.solved-problem-chip {
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 0.95rem;
  border-radius: 16px;
  border: 1px solid rgba(148, 163, 184, 0.22);
  background:
    linear-gradient(180deg, rgba(15, 23, 42, 0.015), rgba(15, 23, 42, 0.04)),
    rgba(255, 255, 255, 0.96);
  text-decoration: none;
  color: inherit;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    box-shadow 160ms ease;
}

.solved-problem-chip:hover,
.solved-problem-chip:focus-visible {
  transform: translateY(-1px);
  border-color: rgba(34, 197, 94, 0.34);
  box-shadow: 0 18px 30px -28px rgba(34, 197, 94, 0.55);
}

.solved-problem-chip__id {
  color: #15803d;
  font-size: 1rem;
  font-weight: 800;
}

.solved-problem-chip__meta {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 0.65rem;
  color: var(--text-muted);
  font-size: 0.88rem;
}

.solved-problem-chip__label {
  color: var(--text-muted);
}

@media (max-width: 960px) {
  .my-info-layout {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
