<template>
  <section class="page-grid single-column">
    <div
      class="my-info-layout"
      :class="{ 'is-summary-only': !showExtendedProfilePanels }"
    >
      <div v-if="showExtendedProfilePanels" class="my-info-side-column">
        <article class="panel my-info-panel my-info-statistics-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">statistics</p>
              <h3>제출 통계</h3>
            </div>

            <StatusBadge
              :label="statisticsStatusLabel"
              :tone="statisticsStatusTone"
            />
          </div>

          <div v-if="shouldShowLoadingState || isStatisticsLoading" class="empty-state">
            <p>제출 통계를 불러오는 중입니다.</p>
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

        <article class="panel my-info-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">recent</p>
              <h3>최근 제출 목록</h3>
            </div>
          </div>

          <div v-if="shouldShowLoadingState || isRecentSubmissionsLoading" class="empty-state">
            <p>최근 제출 목록을 불러오는 중입니다.</p>
          </div>

          <div v-else-if="recentSubmissionsErrorMessage" class="empty-state error-state">
            <p>{{ recentSubmissionsErrorMessage }}</p>
          </div>

          <div v-else-if="recentSubmissions.length === 0" class="empty-state">
            <p>아직 제출한 기록이 없습니다.</p>
          </div>

          <div v-else class="recent-submission-list">
            <RouterLink
              v-for="submission in recentSubmissions"
              :key="submission.submission_id"
              class="recent-submission-item"
              :to="{ name: 'problem-detail', params: { problemId: submission.problem_id } }"
            >
              <span class="recent-submission-item__problem-heading">
                <span class="recent-submission-item__problem-id-group">
                  <span class="recent-submission-item__problem-id">
                    #{{ submission.problem_id }}
                  </span>
                  <span
                    class="recent-submission-item__problem-divider"
                    aria-hidden="true"
                  >
                    ·
                  </span>
                </span>
                <span class="recent-submission-item__problem-title">
                  {{ submission.problem_title }}
                </span>
              </span>
              <SubmissionStatusBadge :status="submission.status" />
              <span class="recent-submission-item__time">
                <span class="recent-submission-relative-time">
                  {{ formatRelativeSubmittedAt(
                    submission.created_at_timestamp,
                    submission.created_at_label
                  ) }}
                  <span
                    v-if="submission.created_at_label"
                    class="recent-submission-time-tooltip"
                  >
                    {{ submission.created_at_label }}
                  </span>
                </span>
              </span>
            </RouterLink>
          </div>
        </article>
      </div>

      <div class="my-info-main-column">
        <article class="panel my-info-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">account</p>
              <h3>정보</h3>
            </div>

            <StatusBadge
              :label="profileStatusLabel"
              :tone="profileStatusTone"
            />
          </div>

          <div v-if="isProfileLoading" class="empty-state">
            <p>정보를 확인하는 중입니다.</p>
          </div>

          <div v-else-if="profileErrorMessage" class="empty-state error-state">
            <p>{{ profileErrorMessage }}</p>
          </div>

          <div v-else-if="!showExtendedProfilePanels" class="empty-state">
            <p>로그인하면 계정 기본 정보를 여기서 확인할 수 있습니다.</p>
          </div>

          <div v-else class="my-info-summary">
            <div class="metric-row">
              <span class="metric-label">계정 번호</span>
              <strong>#{{ displayedUser.user_id }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">ID</span>
              <strong>{{ displayedUser.user_login_id }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">만든 시각</span>
              <strong>{{ formatTimestamp(displayedUser.created_at) }}</strong>
            </div>
            <div v-if="isOwnProfile" class="metric-row">
              <span class="metric-label">제출 상태</span>
              <span class="my-info-role">
                <StatusBadge
                  :label="mySubmissionBanStatusLabel"
                  :tone="mySubmissionBanStatusTone"
                />
              </span>
            </div>
            <div v-if="isOwnProfile" class="metric-row">
              <span class="metric-label">제출 제한</span>
              <strong>{{ mySubmissionBanWindowText }}</strong>
            </div>
            <div v-if="isOwnProfile && mySubmissionBan.submission_banned_until" class="metric-row">
              <span class="metric-label">제출 가능 시각</span>
              <strong>{{ mySubmissionBanUntilText }}</strong>
            </div>
          </div>
        </article>

        <article
          v-if="showExtendedProfilePanels"
          class="panel my-info-panel"
        >
          <div class="panel-header">
            <div>
              <p class="panel-kicker">solved</p>
              <h3>푼 문제 목록</h3>
            </div>

            <StatusBadge
              :label="solvedProblemsStatusLabel"
              :tone="solvedProblemsStatusTone"
            />
          </div>

          <div v-if="shouldShowLoadingState || isSolvedProblemsLoading" class="empty-state">
            <p>푼 문제 목록을 불러오는 중입니다.</p>
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
              <strong
                class="solved-problem-chip__id"
                :class="getProblemStateTextClass(problem.user_problem_state)"
              >
                #{{ problem.problem_id }}
              </strong>
              <div class="solved-problem-chip__meta">
                <span class="solved-problem-chip__label">정답</span>
                <strong>{{ problem.accepted_count }}</strong>
              </div>
            </RouterLink>
          </div>
        </article>

        <article
          v-if="showExtendedProfilePanels"
          class="panel my-info-panel"
        >
          <div class="panel-header">
            <div>
              <p class="panel-kicker">wrong</p>
              <h3>틀린 문제 목록</h3>
            </div>

            <StatusBadge
              :label="wrongProblemsStatusLabel"
              :tone="wrongProblemsStatusTone"
            />
          </div>

          <div v-if="shouldShowLoadingState || isWrongProblemsLoading" class="empty-state">
            <p>틀린 문제 목록을 불러오는 중입니다.</p>
          </div>

          <div v-else-if="wrongProblemsErrorMessage" class="empty-state error-state">
            <p>{{ wrongProblemsErrorMessage }}</p>
          </div>

          <div v-else-if="wrongProblems.length === 0" class="empty-state">
            <p>아직 틀린 문제 목록이 없습니다.</p>
          </div>

          <div v-else class="wrong-problem-grid">
            <RouterLink
              v-for="problem in wrongProblems"
              :key="problem.problem_id"
              class="wrong-problem-chip"
              :to="{ name: 'problem-detail', params: { problemId: problem.problem_id } }"
            >
              <strong
                class="wrong-problem-chip__id"
                :class="getProblemStateTextClass(problem.user_problem_state)"
              >
                #{{ problem.problem_id }}
              </strong>
              <div class="wrong-problem-chip__meta">
                <span class="wrong-problem-chip__label">정답</span>
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
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'

import { getSubmissionList } from '@/api/submission'
import {
  getMySubmissionBan,
  getUserSummary,
  getUserSolvedProblems,
  getUserSubmissionStatistics,
  getUserWrongProblems
} from '@/api/user'
import StatusBadge from '@/components/StatusBadge.vue'
import SubmissionStatusBadge from '@/components/submissions/SubmissionStatusBadge.vue'
import { useAuth } from '@/composables/useAuth'
import { getProblemStateTextClass, normalizeProblemState } from '@/utils/problemState'

const route = useRoute()
const { authState, isAuthenticated, initializeAuth } = useAuth()
const submissionStatistics = ref(null)
const recentSubmissions = ref([])
const solvedProblems = ref([])
const wrongProblems = ref([])
const publicUserSummary = ref(null)
const isStatisticsLoading = ref(true)
const isRecentSubmissionsLoading = ref(true)
const isSolvedProblemsLoading = ref(true)
const isWrongProblemsLoading = ref(true)
const isPublicUserSummaryLoading = ref(false)
const isMySubmissionBanLoading = ref(false)
const statisticsErrorMessage = ref('')
const recentSubmissionsErrorMessage = ref('')
const solvedProblemsErrorMessage = ref('')
const wrongProblemsErrorMessage = ref('')
const publicUserSummaryErrorMessage = ref('')
const mySubmissionBanErrorMessage = ref('')
const mySubmissionBan = ref({
  submission_banned_until: null,
  timestamp: null,
  label: ''
})
let latestStatisticsRequestId = 0
let latestRecentSubmissionsRequestId = 0
let latestSolvedProblemsRequestId = 0
let latestWrongProblemsRequestId = 0
let latestPublicUserSummaryRequestId = 0
let latestMySubmissionBanRequestId = 0
const nowTimestamp = ref(Date.now())
let relativeTimeRefreshTimer = null

const currentUser = computed(() => authState.currentUser ?? {
  id: 0,
  user_login_id: '',
  role_name: 'user',
  permission_level: 0
})

const routeUserLoginId = computed(() => {
  const rawValue = Array.isArray(route.params.userLoginId)
    ? route.params.userLoginId[0]
    : route.params.userLoginId

  return typeof rawValue === 'string' ? rawValue.trim() : ''
})

const isUserProfileRoute = computed(() => route.name === 'user-info')

const requestedProfileUserLoginId = computed(() => {
  if (isUserProfileRoute.value) {
    return routeUserLoginId.value
  }

  if (!authState.initialized || !isAuthenticated.value) {
    return ''
  }

  return currentUser.value.user_login_id ?? ''
})

const activeProfileUserId = computed(() => {
  if (isUserProfileRoute.value) {
    return Number(publicUserSummary.value?.user_id ?? 0)
  }

  if (!authState.initialized || !isAuthenticated.value) {
    return 0
  }

  return Number(currentUser.value.id ?? 0)
})

const isOwnProfile = computed(() => (
  authState.initialized &&
  isAuthenticated.value &&
  (
    (
      isUserProfileRoute.value &&
      routeUserLoginId.value !== '' &&
      routeUserLoginId.value === (currentUser.value.user_login_id ?? '')
    ) ||
    (
      !isUserProfileRoute.value &&
      Number(currentUser.value.id ?? 0) > 0
    )
  )
))

const showExtendedProfilePanels = computed(() => activeProfileUserId.value > 0)

const shouldShowLoadingState = computed(() => (
  !isUserProfileRoute.value &&
  authState.isInitializing &&
  activeProfileUserId.value <= 0
))

const hasPublicProfile = computed(() => (
  Boolean(publicUserSummary.value) &&
  publicUserSummary.value.user_login_id === requestedProfileUserLoginId.value
))

const displayedUser = computed(() => {
  if (hasPublicProfile.value) {
    return publicUserSummary.value
  }

  if (isOwnProfile.value) {
    return {
      user_id: Number(currentUser.value.id ?? 0),
      user_login_id: currentUser.value.user_login_id ?? '',
      created_at: null
    }
  }

  return {
    user_id: 0,
    user_login_id: routeUserLoginId.value,
    created_at: null
  }
})

const isMySubmissionBanActive = computed(() => (
  isOwnProfile.value &&
  typeof mySubmissionBan.value.timestamp === 'number' &&
  !Number.isNaN(mySubmissionBan.value.timestamp) &&
  mySubmissionBan.value.timestamp > nowTimestamp.value
))

const mySubmissionBanStatusLabel = computed(() => {
  if (!isOwnProfile.value) {
    return ''
  }

  if (isMySubmissionBanLoading.value) {
    return '확인 중'
  }

  if (mySubmissionBanErrorMessage.value) {
    return '조회 실패'
  }

  if (isMySubmissionBanActive.value) {
    return '제출 금지'
  }

  return '정상'
})

const mySubmissionBanStatusTone = computed(() => {
  if (!isOwnProfile.value) {
    return 'neutral'
  }

  if (isMySubmissionBanLoading.value) {
    return 'neutral'
  }

  if (mySubmissionBanErrorMessage.value) {
    return 'warning'
  }

  return isMySubmissionBanActive.value ? 'danger' : 'success'
})

const mySubmissionBanWindowText = computed(() => {
  if (!isOwnProfile.value) {
    return ''
  }

  if (isMySubmissionBanLoading.value) {
    return '제출 제한 상태를 확인하는 중입니다.'
  }

  if (mySubmissionBanErrorMessage.value) {
    return mySubmissionBanErrorMessage.value
  }

  if (!mySubmissionBan.value.submission_banned_until) {
    return '현재 제출 제한이 없습니다.'
  }

  if (isMySubmissionBanActive.value) {
    return `${formatRelativeRemainingTime(mySubmissionBan.value.timestamp)} 남음`
  }

  return '이전 제출 제한이 만료되었습니다.'
})

const mySubmissionBanUntilText = computed(() => {
  if (
    !isOwnProfile.value ||
    typeof mySubmissionBan.value.submission_banned_until !== 'string' ||
    !mySubmissionBan.value.submission_banned_until
  ) {
    return '-'
  }

  return formatTimestamp(mySubmissionBan.value.submission_banned_until)
})

const isProfileLoading = computed(() => {
  if (shouldShowLoadingState.value) {
    return true
  }

  if (!showExtendedProfilePanels.value) {
    return false
  }

  return isPublicUserSummaryLoading.value
})

const profileErrorMessage = computed(() => {
  if (isUserProfileRoute.value) {
    return publicUserSummaryErrorMessage.value
  }

  if (!showExtendedProfilePanels.value) {
    return ''
  }

  return publicUserSummaryErrorMessage.value
})

const profileStatusLabel = computed(() => {
  if (shouldShowLoadingState.value || isPublicUserSummaryLoading.value) {
    return 'Loading'
  }

  if (!showExtendedProfilePanels.value) {
    return 'Guest'
  }

  if (publicUserSummaryErrorMessage.value) {
    return 'Error'
  }

  return isOwnProfile.value ? 'Signed In' : 'Public'
})

const profileStatusTone = computed(() => {
  if (shouldShowLoadingState.value || isPublicUserSummaryLoading.value) {
    return 'neutral'
  }

  if (!showExtendedProfilePanels.value) {
    return 'neutral'
  }

  if (publicUserSummaryErrorMessage.value) {
    return 'danger'
  }

  return isOwnProfile.value ? 'success' : 'neutral'
})

const statisticsStatusLabel = computed(() => {
  if (shouldShowLoadingState.value || isStatisticsLoading.value) {
    return 'Loading'
  }

  if (!showExtendedProfilePanels.value) {
    return 'Guest'
  }

  return statisticsErrorMessage.value ? 'Error' : 'Loaded'
})

const statisticsStatusTone = computed(() => {
  if (shouldShowLoadingState.value || isStatisticsLoading.value) {
    return 'neutral'
  }

  if (!showExtendedProfilePanels.value) {
    return 'neutral'
  }

  if (statisticsErrorMessage.value) {
    return 'danger'
  }

  return 'success'
})

const solvedProblemsStatusLabel = computed(() => {
  if (shouldShowLoadingState.value || isSolvedProblemsLoading.value) {
    return 'Loading'
  }

  if (!showExtendedProfilePanels.value) {
    return 'Guest'
  }

  if (solvedProblemsErrorMessage.value) {
    return 'Error'
  }

  return `${solvedProblems.value.length} Solved`
})

const solvedProblemsStatusTone = computed(() => {
  if (shouldShowLoadingState.value || isSolvedProblemsLoading.value) {
    return 'neutral'
  }

  if (!showExtendedProfilePanels.value) {
    return 'neutral'
  }

  if (solvedProblemsErrorMessage.value) {
    return 'danger'
  }

  return 'success'
})

const recentSubmissionsStatusLabel = computed(() => {
  if (shouldShowLoadingState.value || isRecentSubmissionsLoading.value) {
    return 'Loading'
  }

  if (!showExtendedProfilePanels.value) {
    return 'Guest'
  }

  if (recentSubmissionsErrorMessage.value) {
    return 'Error'
  }

  return `${recentSubmissions.value.length} Recent`
})

const recentSubmissionsStatusTone = computed(() => {
  if (shouldShowLoadingState.value || isRecentSubmissionsLoading.value) {
    return 'neutral'
  }

  if (!showExtendedProfilePanels.value) {
    return 'neutral'
  }

  if (recentSubmissionsErrorMessage.value) {
    return 'danger'
  }

  return 'success'
})

const wrongProblemsStatusLabel = computed(() => {
  if (shouldShowLoadingState.value || isWrongProblemsLoading.value) {
    return 'Loading'
  }

  if (!showExtendedProfilePanels.value) {
    return 'Guest'
  }

  if (wrongProblemsErrorMessage.value) {
    return 'Error'
  }

  return `${wrongProblems.value.length} Wrong`
})

const wrongProblemsStatusTone = computed(() => {
  if (shouldShowLoadingState.value || isWrongProblemsLoading.value) {
    return 'neutral'
  }

  if (!showExtendedProfilePanels.value) {
    return 'neutral'
  }

  if (wrongProblemsErrorMessage.value) {
    return 'danger'
  }

  return 'danger'
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
      accepted_count: normalizeCount(problem?.accepted_count),
      user_problem_state: normalizeProblemState(problem?.user_problem_state)
    }))
    .filter((problem) => problem.problem_id > 0)
    .sort((leftProblem, rightProblem) => leftProblem.problem_id - rightProblem.problem_id)
}

function normalizeRecentSubmissions(payload){
  const submissionValues = Array.isArray(payload?.submissions)
    ? payload.submissions
    : []

  return submissionValues
    .map((submission) => {
      const submittedAt = normalizeSubmittedAt(submission?.created_at)

      return {
        submission_id: Number(submission?.submission_id ?? 0),
        problem_id: Number(submission?.problem_id ?? 0),
        problem_title: typeof submission?.problem_title === 'string'
          ? submission.problem_title
          : '제목 없음',
        status: typeof submission?.status === 'string' ? submission.status : '',
        created_at_timestamp: submittedAt.timestamp,
        created_at_label: submittedAt.label
      }
    })
    .filter((submission) =>
      submission.submission_id > 0 &&
      submission.problem_id > 0 &&
      submission.status
    )
}

function normalizeWrongProblems(payload){
  const wrongProblemValues = Array.isArray(payload?.wrong_problems)
    ? payload.wrong_problems
    : []

  return wrongProblemValues
    .map((problem) => ({
      problem_id: Number(problem?.problem_id ?? 0),
      accepted_count: normalizeCount(problem?.accepted_count),
      user_problem_state: normalizeProblemState(problem?.user_problem_state)
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

function formatRelativeRemainingTime(timestamp){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return '-'
  }

  const remainingSeconds = Math.max(1, Math.floor((timestamp - nowTimestamp.value) / 1000))
  if (remainingSeconds < 60) {
    return `${remainingSeconds}초`
  }

  const remainingMinutes = Math.floor(remainingSeconds / 60)
  if (remainingMinutes < 60) {
    return `${remainingMinutes}분`
  }

  const remainingHours = Math.floor(remainingMinutes / 60)
  if (remainingHours < 24) {
    return `${remainingHours}시간`
  }

  const remainingDays = Math.floor(remainingHours / 24)
  if (remainingDays < 30) {
    return `${remainingDays}일`
  }

  const remainingMonths = Math.floor(remainingDays / 30)
  if (remainingMonths < 12) {
    return `${remainingMonths}달`
  }

  return `${Math.floor(remainingDays / 365)}년`
}

function normalizeSubmittedAt(value){
  if (typeof value !== 'string' || !value.trim()) {
    return {
      timestamp: null,
      label: ''
    }
  }

  const trimmedValue = value.trim()
  const matchedTimestamp = trimmedValue.match(
    /^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2}:\d{2})(?:\.(\d{1,6}))?([+-]\d{2})(?::?(\d{2}))?$/
  )

  if (matchedTimestamp) {
    const [, datePart, timePart, fractionPart = '', offsetHour, offsetMinute = '00'] =
      matchedTimestamp
    const normalizedFraction = fractionPart
      ? `.${fractionPart.slice(0, 3).padEnd(3, '0')}`
      : ''
    const parsedTimestamp = Date.parse(
      `${datePart}T${timePart}${normalizedFraction}${offsetHour}:${offsetMinute}`
    )

    return {
      timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
      label: `${datePart} ${timePart}`
    }
  }

  const parsedTimestamp = Date.parse(trimmedValue.replace(' ', 'T'))
  return {
    timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
    label: trimmedValue
  }
}

function formatRelativeSubmittedAt(timestamp, fallbackLabel = '-'){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return fallbackLabel || '-'
  }

  const elapsedSeconds = Math.max(1, Math.floor((nowTimestamp.value - timestamp) / 1000))

  if (elapsedSeconds < 60) {
    return `${elapsedSeconds}초 전`
  }

  const elapsedMinutes = Math.floor(elapsedSeconds / 60)
  if (elapsedMinutes < 60) {
    return `${elapsedMinutes}분 전`
  }

  const elapsedHours = Math.floor(elapsedMinutes / 60)
  if (elapsedHours < 24) {
    return `${elapsedHours}시간 전`
  }

  const elapsedDays = Math.floor(elapsedHours / 24)
  if (elapsedDays < 30) {
    return `${elapsedDays}일 전`
  }

  const elapsedMonths = Math.floor(elapsedDays / 30)
  if (elapsedMonths < 12) {
    return `${elapsedMonths}달 전`
  }

  const elapsedYears = Math.floor(elapsedDays / 365)
  return `${elapsedYears}년 전`
}

function startRelativeTimeRefresh(){
  nowTimestamp.value = Date.now()

  if (relativeTimeRefreshTimer) {
    clearInterval(relativeTimeRefreshTimer)
  }

  relativeTimeRefreshTimer = window.setInterval(() => {
    nowTimestamp.value = Date.now()
  }, 30_000)
}

function stopRelativeTimeRefresh(){
  if (relativeTimeRefreshTimer) {
    clearInterval(relativeTimeRefreshTimer)
    relativeTimeRefreshTimer = null
  }
}

async function loadSubmissionStatistics(){
  const profileUserId = activeProfileUserId.value

  if (profileUserId <= 0) {
    submissionStatistics.value = null
    statisticsErrorMessage.value = ''
    isStatisticsLoading.value = false
    return
  }

  const requestId = ++latestStatisticsRequestId
  isStatisticsLoading.value = true
  statisticsErrorMessage.value = ''

  try {
    const payload = await getUserSubmissionStatistics(profileUserId)
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

async function loadRecentSubmissions(){
  const profileUserId = activeProfileUserId.value

  if (profileUserId <= 0) {
    recentSubmissions.value = []
    recentSubmissionsErrorMessage.value = ''
    isRecentSubmissionsLoading.value = false
    return
  }

  const requestId = ++latestRecentSubmissionsRequestId
  isRecentSubmissionsLoading.value = true
  recentSubmissionsErrorMessage.value = ''

  try {
    const payload = await getSubmissionList({
      userId: profileUserId,
      limit: 10,
      bearerToken: authState.token || undefined
    })
    if (requestId !== latestRecentSubmissionsRequestId) {
      return
    }

    recentSubmissions.value = normalizeRecentSubmissions(payload)
  } catch (error) {
    if (requestId !== latestRecentSubmissionsRequestId) {
      return
    }

    recentSubmissions.value = []
    recentSubmissionsErrorMessage.value = error instanceof Error
      ? error.message
      : '최근 제출 목록을 불러오지 못했습니다.'
  } finally {
    if (requestId === latestRecentSubmissionsRequestId) {
      isRecentSubmissionsLoading.value = false
    }
  }
}

async function loadSolvedProblems(){
  const profileUserId = activeProfileUserId.value

  if (profileUserId <= 0) {
    solvedProblems.value = []
    solvedProblemsErrorMessage.value = ''
    isSolvedProblemsLoading.value = false
    return
  }

  const requestId = ++latestSolvedProblemsRequestId
  isSolvedProblemsLoading.value = true
  solvedProblemsErrorMessage.value = ''

  try {
    const payload = await getUserSolvedProblems(
      profileUserId,
      authState.token || undefined
    )
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

async function loadWrongProblems(){
  const profileUserId = activeProfileUserId.value

  if (profileUserId <= 0) {
    wrongProblems.value = []
    wrongProblemsErrorMessage.value = ''
    isWrongProblemsLoading.value = false
    return
  }

  const requestId = ++latestWrongProblemsRequestId
  isWrongProblemsLoading.value = true
  wrongProblemsErrorMessage.value = ''

  try {
    const payload = await getUserWrongProblems(
      profileUserId,
      authState.token || undefined
    )
    if (requestId !== latestWrongProblemsRequestId) {
      return
    }

    wrongProblems.value = normalizeWrongProblems(payload)
  } catch (error) {
    if (requestId !== latestWrongProblemsRequestId) {
      return
    }

    wrongProblems.value = []
    wrongProblemsErrorMessage.value = error instanceof Error
      ? error.message
      : '틀린 문제 목록을 불러오지 못했습니다.'
  } finally {
    if (requestId === latestWrongProblemsRequestId) {
      isWrongProblemsLoading.value = false
    }
  }
}

function normalizeUserSummary(payload){
  const normalizedUserLoginId =
    typeof payload?.user_login_id === 'string' ? payload.user_login_id : ''

  return {
    user_id: Number(payload?.user_id ?? 0),
    user_login_id: normalizedUserLoginId,
    created_at: typeof payload?.created_at === 'string' ? payload.created_at : null
  }
}

async function loadPublicUserSummary(userLoginId){
  if (typeof userLoginId !== 'string' || !userLoginId.trim()) {
    publicUserSummary.value = null
    publicUserSummaryErrorMessage.value = '유효하지 않은 사용자입니다.'
    isPublicUserSummaryLoading.value = false
    return
  }

  const requestId = ++latestPublicUserSummaryRequestId
  isPublicUserSummaryLoading.value = true
  publicUserSummary.value = null
  publicUserSummaryErrorMessage.value = ''

  try {
    const payload = await getUserSummary(userLoginId.trim())
    if (requestId !== latestPublicUserSummaryRequestId) {
      return
    }

    publicUserSummary.value = normalizeUserSummary(payload)
  } catch (error) {
    if (requestId !== latestPublicUserSummaryRequestId) {
      return
    }

    publicUserSummary.value = null
    publicUserSummaryErrorMessage.value = error instanceof Error
      ? error.message
      : '사용자 정보를 불러오지 못했습니다.'
  } finally {
    if (requestId === latestPublicUserSummaryRequestId) {
      isPublicUserSummaryLoading.value = false
    }
  }
}

async function loadMySubmissionBan(){
  if (!isOwnProfile.value || !authState.token) {
    latestMySubmissionBanRequestId += 1
    mySubmissionBan.value = {
      submission_banned_until: null,
      timestamp: null,
      label: ''
    }
    mySubmissionBanErrorMessage.value = ''
    isMySubmissionBanLoading.value = false
    return
  }

  const requestId = ++latestMySubmissionBanRequestId
  isMySubmissionBanLoading.value = true
  mySubmissionBanErrorMessage.value = ''

  try {
    const payload = await getMySubmissionBan(authState.token)
    if (requestId !== latestMySubmissionBanRequestId) {
      return
    }

    const submissionBannedUntil =
      typeof payload?.submission_banned_until === 'string'
        ? payload.submission_banned_until
        : null
    const normalizedSubmissionBan = normalizeSubmittedAt(submissionBannedUntil)

    mySubmissionBan.value = {
      submission_banned_until: submissionBannedUntil,
      timestamp: normalizedSubmissionBan.timestamp,
      label: normalizedSubmissionBan.label
    }
  } catch (error) {
    if (requestId !== latestMySubmissionBanRequestId) {
      return
    }

    mySubmissionBan.value = {
      submission_banned_until: null,
      timestamp: null,
      label: ''
    }
    mySubmissionBanErrorMessage.value = error instanceof Error
      ? error.message
      : '제출 제한 상태를 불러오지 못했습니다.'
  } finally {
    if (requestId === latestMySubmissionBanRequestId) {
      isMySubmissionBanLoading.value = false
    }
  }
}

watch(
  () => [
    activeProfileUserId.value,
    authState.initialized,
    authState.token,
    currentUser.value.id
  ],
  ([profileUserId, initialized]) => {
    if (!isUserProfileRoute.value && !initialized) {
      isStatisticsLoading.value = true
      isRecentSubmissionsLoading.value = true
      isSolvedProblemsLoading.value = true
      isWrongProblemsLoading.value = true
      return
    }

    if (profileUserId <= 0) {
      latestStatisticsRequestId += 1
      latestRecentSubmissionsRequestId += 1
      latestSolvedProblemsRequestId += 1
      latestWrongProblemsRequestId += 1
      submissionStatistics.value = null
      recentSubmissions.value = []
      solvedProblems.value = []
      wrongProblems.value = []
      statisticsErrorMessage.value = ''
      recentSubmissionsErrorMessage.value = ''
      solvedProblemsErrorMessage.value = ''
      wrongProblemsErrorMessage.value = ''
      isStatisticsLoading.value = false
      isRecentSubmissionsLoading.value = false
      isSolvedProblemsLoading.value = false
      isWrongProblemsLoading.value = false
      return
    }

    loadSubmissionStatistics()
    loadRecentSubmissions()
    loadSolvedProblems()
    loadWrongProblems()
  },
  {
    immediate: true
  }
)

watch(
  () => [
    isOwnProfile.value,
    authState.initialized,
    authState.token,
    currentUser.value.id
  ],
  ([ownProfile, initialized]) => {
    if (!initialized) {
      isMySubmissionBanLoading.value = ownProfile
      return
    }

    if (!ownProfile || !authState.token) {
      latestMySubmissionBanRequestId += 1
      mySubmissionBan.value = {
        submission_banned_until: null,
        timestamp: null,
        label: ''
      }
      mySubmissionBanErrorMessage.value = ''
      isMySubmissionBanLoading.value = false
      return
    }

    loadMySubmissionBan()
  },
  {
    immediate: true
  }
)

watch(
  () => [
    requestedProfileUserLoginId.value,
    isUserProfileRoute.value,
    authState.initialized
  ],
  ([profileUserLoginId, , initialized]) => {
    if (!isUserProfileRoute.value && !initialized) {
      isPublicUserSummaryLoading.value = true
      return
    }

    if (!profileUserLoginId) {
      latestPublicUserSummaryRequestId += 1
      publicUserSummary.value = null
      publicUserSummaryErrorMessage.value = ''
      isPublicUserSummaryLoading.value = false
      return
    }

    loadPublicUserSummary(profileUserLoginId)
  },
  {
    immediate: true
  }
)

onMounted(() => {
  initializeAuth()
  startRelativeTimeRefresh()
})

onBeforeUnmount(() => {
  stopRelativeTimeRefresh()
})
</script>

<style scoped>
.my-info-layout {
  display: grid;
  grid-template-columns: minmax(364px, 468px) minmax(0, 1fr);
  gap: 1.25rem;
  align-items: start;
}

.my-info-layout.is-summary-only {
  grid-template-columns: minmax(0, 1fr);
}

.my-info-side-column {
  display: grid;
  gap: 1.25rem;
  align-content: start;
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

.recent-submission-list {
  display: grid;
  gap: 0.75rem;
}

.recent-submission-item {
  display: flex;
  align-items: center;
  gap: 0.8rem;
  padding: 0.95rem 1rem;
  border-radius: 16px;
  border: 1px solid rgba(148, 163, 184, 0.18);
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

.recent-submission-item:hover,
.recent-submission-item:focus-visible {
  transform: translateY(-1px);
  border-color: rgba(37, 99, 235, 0.28);
  box-shadow: 0 18px 30px -28px rgba(37, 99, 235, 0.5);
}

.recent-submission-item__problem-heading {
  display: flex;
  align-items: baseline;
  gap: 0.75rem;
  min-width: 0;
  flex: 1 1 auto;
  font-size: 1rem;
  line-height: 1.12;
  letter-spacing: -0.02em;
}

.recent-submission-item__problem-id-group {
  display: inline-flex;
  align-items: baseline;
  gap: 0.5rem;
  white-space: nowrap;
  color: var(--ink-soft);
  flex-shrink: 0;
}

.recent-submission-item__problem-id {
  font-weight: 800;
  letter-spacing: -0.03em;
}

.recent-submission-item__problem-divider {
  color: rgba(20, 33, 61, 0.34);
  font-weight: 700;
}

.recent-submission-item__problem-title {
  font-weight: 700;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--ink-strong);
}

.recent-submission-item__time {
  color: #6C8BCF;
  font-size: 0.9rem;
  font-weight: 600;
  white-space: nowrap;
}

.recent-submission-relative-time {
  position: relative;
  display: inline-flex;
  align-items: center;
  color: inherit;
  cursor: help;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 140ms ease,
    text-decoration-color 140ms ease,
    transform 140ms ease;
}

.recent-submission-relative-time:hover {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.recent-submission-time-tooltip {
  position: absolute;
  right: 0;
  bottom: calc(100% + 0.55rem);
  z-index: 10;
  min-width: max-content;
  max-width: 220px;
  padding: 0.55rem 0.7rem;
  border: 1px solid rgba(20, 33, 61, 0.12);
  border-radius: 12px;
  background: rgba(20, 33, 61, 0.96);
  box-shadow: 0 14px 32px rgba(20, 33, 61, 0.18);
  color: #f8fafc;
  font-size: 0.8rem;
  font-weight: 600;
  line-height: 1.4;
  white-space: nowrap;
  pointer-events: none;
  opacity: 0;
  transform: translateY(6px);
  transition:
    opacity 140ms ease,
    transform 140ms ease;
}

.recent-submission-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.recent-submission-relative-time:hover .recent-submission-time-tooltip {
  opacity: 1;
  transform: translateY(0);
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
  color: var(--ink-strong);
  font-size: 1rem;
  font-weight: 800;
}

.solved-problem-chip__id.problem-state-text--solved {
  color: #15803d;
}

.solved-problem-chip__id.problem-state-text--wrong {
  color: #b91c1c;
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

.wrong-problem-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(116px, 1fr));
  gap: 0.8rem;
  max-height: 28rem;
  overflow: auto;
  padding-right: 0.2rem;
}

.wrong-problem-chip {
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

.wrong-problem-chip:hover,
.wrong-problem-chip:focus-visible {
  transform: translateY(-1px);
  border-color: rgba(239, 68, 68, 0.34);
  box-shadow: 0 18px 30px -28px rgba(239, 68, 68, 0.55);
}

.wrong-problem-chip__id {
  color: var(--ink-strong);
  font-size: 1rem;
  font-weight: 800;
}

.wrong-problem-chip__id.problem-state-text--solved {
  color: #15803d;
}

.wrong-problem-chip__id.problem-state-text--wrong {
  color: #b91c1c;
}

.wrong-problem-chip__meta {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 0.65rem;
  color: var(--text-muted);
  font-size: 0.88rem;
}

.wrong-problem-chip__label {
  color: var(--text-muted);
}

@media (max-width: 960px) {
  .my-info-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .recent-submission-item {
    gap: 0.65rem;
  }
}
</style>
