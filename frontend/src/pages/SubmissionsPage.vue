<template>
  <section class="page-grid single-column">
    <article class="panel submissions-panel">
      <div class="submissions-toolbar">
        <div>
          <p class="panel-kicker">submissions</p>
          <h3>{{ pageTitle }}</h3>
        </div>

        <div class="submissions-toolbar-actions">
          <div class="submission-summary-group">
            <StatusBadge
              :label="isLoading ? 'Loading' : `${formatCount(totalSubmissionCount)} submissions`"
              :tone="errorMessage ? 'danger' : 'success'"
            />
            <span
              v-if="!isLoading && totalSubmissionCount"
              class="submission-summary-text"
            >
              {{ visibleRangeText }}
            </span>
          </div>

          <RouterLink
            v-if="numericProblemId"
            class="submissions-back-link"
            :to="{ name: 'problem-detail', params: { problemId: numericProblemId } }"
          >
            문제로 돌아가기
          </RouterLink>
        </div>
      </div>

      <div class="submission-filter-bar">
        <div class="submission-filter-fields">
          <div
            v-if="!hasFixedProblemId"
            class="submission-filter-group"
          >
            <label class="submission-filter-label" for="submission-problem-filter">문제 번호</label>
            <input
              id="submission-problem-filter"
              v-model="selectedProblemIdFilter"
              class="submission-filter-input"
              type="number"
              inputmode="numeric"
              min="1"
              placeholder="문제 번호"
              :disabled="isLoading"
            />
          </div>

          <div class="submission-filter-group">
            <label class="submission-filter-label" for="submission-status-filter">상태 필터</label>
            <select
              id="submission-status-filter"
              v-model="selectedStatusFilter"
              class="submission-filter-select"
              :disabled="isLoading"
            >
              <option
                v-for="option in submissionStatusOptions"
                :key="option.value || 'all'"
                :value="option.value"
              >
                {{ option.label }}
              </option>
            </select>
          </div>

          <div class="submission-filter-group">
            <label class="submission-filter-label" for="submission-language-filter">언어 필터</label>
            <select
              id="submission-language-filter"
              v-model="selectedLanguageFilter"
              class="submission-filter-select"
              :disabled="isLoadingLanguages"
            >
              <option
                v-for="option in submissionLanguageFilterOptions"
                :key="option.value || 'all'"
                :value="option.value"
              >
                {{ option.label }}
              </option>
            </select>
          </div>
        </div>

        <div class="submission-filter-actions">
          <button
            type="button"
            class="ghost-button"
            :disabled="isLoading || !canResetFilters"
            @click="resetSubmissionFilters"
          >
            초기화
          </button>
          <button
            type="button"
            class="primary-button"
            :disabled="isLoading || !canApplyFilters"
            @click="applySubmissionFilters"
          >
            적용
          </button>
        </div>
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>제출 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else-if="!totalSubmissionCount" class="empty-state">
        <p>등록된 제출이 아직 없습니다.</p>
      </div>

      <div v-else class="submission-table-wrapper">
        <div
          class="submission-table"
          :class="{ 'has-actions': canManageSubmissionRejudge }"
        >
          <div
            class="submission-table-head"
            :class="{ 'has-actions': canManageSubmissionRejudge }"
          >
            <span>제출번호</span>
            <span class="submission-table-head-user">닉네임</span>
            <span>문제번호</span>
            <span>결과</span>
            <span>실행시간</span>
            <span>메모리</span>
            <span>언어</span>
            <span>제출 시각</span>
            <span v-if="canManageSubmissionRejudge" class="submission-head-action">채점 내역</span>
            <span v-if="canManageSubmissionRejudge" class="submission-head-action">재채점</span>
          </div>

          <div
            v-for="submission in submissions"
            :key="submission.submission_id"
            class="submission-row"
            :class="{ 'has-actions': canManageSubmissionRejudge }"
          >
            <span class="submission-cell is-number">
              {{ formatCount(submission.submission_id) }}
            </span>
            <span class="submission-cell is-user">
              {{ submission.user_name }}
            </span>
            <RouterLink
              class="submission-problem-link"
              :class="getProblemStateTextClass(submission.user_problem_state)"
              :to="{ name: 'problem-detail', params: { problemId: submission.problem_id } }"
            >
              #{{ formatCount(submission.problem_id) }}
            </RouterLink>
            <div class="submission-cell">
              <StatusBadge
                :label="getStatusLabel(submission.status)"
                :tone="getStatusTone(submission.status)"
              />
            </div>
            <span class="submission-cell is-metric">
              {{ formatElapsedMs(submission.elapsed_ms) }}
            </span>
            <span class="submission-cell is-metric">
              {{ formatMemory(submission.max_rss_kb) }}
            </span>
            <button
              v-if="canViewSource(submission)"
              type="button"
              class="submission-language-button"
              @click="openSourceDialog(submission)"
            >
              {{ submission.language }}
            </button>
            <span v-else class="submission-cell is-language">
              {{ submission.language }}
            </span>
            <span class="submission-cell is-submitted-at">
              <span
                class="submission-relative-time"
                tabindex="0"
              >
                {{ formatRelativeSubmittedAt(submission.created_at_timestamp) }}
                <span
                  v-if="submission.created_at_label"
                  class="submission-time-tooltip"
                >
                  {{ submission.created_at_label }}
                </span>
              </span>
            </span>
            <div v-if="canManageSubmissionRejudge" class="submission-cell submission-action-cell">
              <button
                type="button"
                class="ghost-button submission-history-button"
                @click="openHistoryDialog(submission)"
              >
                채점 내역
              </button>
            </div>
            <div v-if="canManageSubmissionRejudge" class="submission-cell submission-action-cell">
              <button
                v-if="canRejudgeSubmission(submission)"
                type="button"
                class="ghost-button submission-rejudge-button"
                :disabled="isRejudgingSubmission(submission.submission_id)"
                @click="handleRejudgeSubmission(submission)"
              >
                {{ isRejudgingSubmission(submission.submission_id) ? '요청 중...' : '재채점' }}
              </button>
            </div>
          </div>
        </div>
      </div>

      <div
        v-if="!isLoading && !errorMessage && totalSubmissionCount > listLimit"
        class="submission-pagination"
      >
        <div class="pagination-controls">
          <button
            type="button"
            class="ghost-button pagination-button"
            :disabled="currentPage === 1"
            @click="goToPage(currentPage - 1)"
          >
            이전
          </button>

          <div class="pagination-pages">
            <template
              v-for="item in paginationItems"
              :key="item.key"
            >
              <button
                v-if="item.type === 'page'"
                type="button"
                class="pagination-page"
                :class="{ 'is-active': item.value === currentPage }"
                @click="goToPage(item.value)"
              >
                {{ item.value }}
              </button>
              <span
                v-else
                class="pagination-ellipsis"
                aria-hidden="true"
              >
                ...
              </span>
            </template>
          </div>

          <button
            type="button"
            class="ghost-button pagination-button"
            :disabled="currentPage === totalPages"
            @click="goToPage(currentPage + 1)"
          >
            다음
          </button>
        </div>

        <form class="pagination-jump" @submit.prevent="submitPageJump">
          <label class="sr-only" for="submission-page-jump">페이지 이동</label>
          <input
            id="submission-page-jump"
            v-model="pageJumpInput"
            class="pagination-jump-input"
            type="number"
            inputmode="numeric"
            min="1"
            :max="totalPages"
            :placeholder="`1-${totalPages}`"
          />
          <button
            type="submit"
            class="ghost-button pagination-jump-button"
            :disabled="isLoading"
          >
            이동
          </button>
        </form>
      </div>
    </article>
  </section>

  <Teleport to="body">
    <div
      v-if="historyDialogOpen"
      class="submission-history-backdrop"
      @pointerdown="handleHistoryBackdropPointerDown"
      @click.self="handleHistoryBackdropClick"
    >
      <section
        class="submission-history-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="submission-history-title"
      >
        <div class="submission-history-header">
          <div>
            <p class="panel-kicker">submission history</p>
            <h3 id="submission-history-title">
              제출 #{{ activeHistorySubmissionId ? formatCount(activeHistorySubmissionId) : '' }} 채점 내역
            </h3>
          </div>
          <button
            type="button"
            class="icon-button"
            aria-label="닫기"
            @click="closeHistoryDialog"
          >
            ×
          </button>
        </div>

        <div v-if="isLoadingHistory" class="empty-state">
          <p>채점 내역을 불러오는 중입니다.</p>
        </div>

        <div v-else-if="historyErrorMessage" class="empty-state error-state">
          <p>{{ historyErrorMessage }}</p>
        </div>

        <template v-else>
          <p
            v-if="shouldPollSubmissionHistory"
            class="submission-history-live"
          >
            자동 갱신 중
          </p>

          <div
            v-if="submissionHistoryEntries.length"
            class="submission-history-timeline"
          >
            <article
              v-for="(historyEntry, historyIndex) in submissionHistoryEntries"
              :key="historyEntry.history_key"
              class="submission-history-entry"
              :class="{ 'is-current': historyIndex === submissionHistoryEntries.length - 1 }"
            >
              <span class="submission-history-entry-marker" aria-hidden="true"></span>
              <div class="submission-history-entry-card">
                <div class="submission-history-entry-top">
                  <StatusBadge
                    :label="getStatusLabel(historyEntry.to_status)"
                    :tone="getStatusTone(historyEntry.to_status)"
                  />
                  <span class="submission-history-entry-time">
                    {{ historyEntry.created_at_label }}
                  </span>
                </div>
                <p class="submission-history-entry-transition">
                  {{ formatHistoryTransition(historyEntry) }}
                </p>
                <p
                  v-if="historyEntry.reason"
                  class="submission-history-entry-reason"
                >
                  사유: {{ historyEntry.reason }}
                </p>
              </div>
            </article>
          </div>

          <div v-else class="empty-state submission-history-empty-state">
            <p>히스토리가 아직 없습니다.</p>
          </div>
        </template>
      </section>
    </div>
  </Teleport>

  <Teleport to="body">
    <div
      v-if="sourceDialogOpen"
      class="submission-source-backdrop"
      @pointerdown="handleSourceBackdropPointerDown"
      @click.self="handleSourceBackdropClick"
    >
      <section
        class="submission-source-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="submission-source-title"
      >
        <div class="submission-source-header">
          <div>
            <p class="panel-kicker">submission source</p>
            <h3 id="submission-source-title">
              제출 #{{ activeSourceSubmissionId ? formatCount(activeSourceSubmissionId) : '' }} 소스 코드
            </h3>
          </div>
          <button
            type="button"
            class="icon-button"
            aria-label="닫기"
            @click="closeSourceDialog"
          >
            ×
          </button>
        </div>

        <div v-if="isLoadingSource" class="empty-state">
          <p>소스 코드를 불러오는 중입니다.</p>
        </div>

        <div v-else-if="sourceErrorMessage" class="empty-state error-state">
          <p>{{ sourceErrorMessage }}</p>
        </div>

        <div v-else-if="sourceDetail" class="submission-source-content">
          <div class="submission-source-meta">
            <StatusBadge :label="sourceDetail.language" tone="neutral" />
          </div>
          <pre class="submission-source-code"><code>{{ sourceDetail.source_code }}</code></pre>
          <div
            v-if="sourceDetail.compile_output || sourceDetail.judge_output"
            class="submission-source-diagnostics"
          >
            <div
              v-if="sourceDetail.compile_output"
              class="submission-source-diagnostic"
            >
              <p class="submission-source-diagnostic-title">컴파일 출력</p>
              <pre class="submission-source-diagnostic-log"><code>{{ sourceDetail.compile_output }}</code></pre>
            </div>
            <div
              v-if="sourceDetail.judge_output"
              class="submission-source-diagnostic"
            >
              <p class="submission-source-diagnostic-title">채점 출력</p>
              <pre class="submission-source-diagnostic-log"><code>{{ sourceDetail.judge_output }}</code></pre>
            </div>
          </div>
          <div class="submission-source-actions">
            <button
              type="button"
              class="ghost-button"
              @click="copySourceCode"
            >
              {{ copyButtonLabel }}
            </button>
          </div>
        </div>
      </section>
    </div>
  </Teleport>
</template>

<script setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getSupportedLanguages } from '@/api/http'
import {
  getSubmissionDetail,
  getSubmissionHistory,
  getSubmissionList,
  getSubmissionSource,
  rejudgeSubmission
} from '@/api/submission'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'
import { useNotice } from '@/composables/useNotice'
import { getProblemStateTextClass } from '@/utils/problemState'

const route = useRoute()
const router = useRouter()
const { authState, isAuthenticated, initializeAuth } = useAuth()
const { showErrorNotice, showSuccessNotice } = useNotice()
const listLimit = 50
const submissionPollingIntervalMs = 2000
const pollingSubmissionStatuses = new Set(['queued', 'judging'])
const submissionStatusOptions = [
  { value: '', label: '전체' },
  { value: 'accepted', label: '정답' },
  { value: 'wrong_answer', label: '오답' },
  { value: 'time_limit_exceeded', label: '시간 초과' },
  { value: 'memory_limit_exceeded', label: '메모리 초과' },
  { value: 'runtime_error', label: '런타임 에러' },
  { value: 'output_exceeded', label: '출력 초과' },
  { value: 'compile_error', label: '컴파일 에러' }
]
const isLoading = ref(true)
const isLoadingLanguages = ref(true)
const errorMessage = ref('')
const actionMessage = ref('')
const actionErrorMessage = ref('')
const submissions = ref([])
const hasLoadedOnce = ref(false)
const currentPage = ref(1)
const pageJumpInput = ref('')
const totalSubmissionCount = ref(0)
const historyDialogOpen = ref(false)
const isLoadingHistory = ref(false)
const historyErrorMessage = ref('')
const submissionHistoryEntries = ref([])
const activeHistorySubmissionId = ref(null)
const isHistoryBackdropInteraction = ref(false)
const sourceDialogOpen = ref(false)
const isLoadingSource = ref(false)
const sourceErrorMessage = ref('')
const sourceDetail = ref(null)
const activeSourceSubmissionId = ref(null)
const copyState = ref('idle')
const isSourceBackdropInteraction = ref(false)
const nowTimestamp = ref(Date.now())
const rejudgingSubmissionIds = ref([])
const isDocumentVisible = ref(typeof document === 'undefined' ? true : !document.hidden)
const selectedProblemIdFilter = ref('')
const selectedStatusFilter = ref('')
const selectedLanguageFilter = ref('')
const supportedSubmissionLanguages = ref([])
const countFormatter = new Intl.NumberFormat()
const authenticatedBearerToken = computed(() =>
  authState.initialized && isAuthenticated.value ? authState.token : ''
)
const canManageSubmissionRejudge = computed(() =>
  Number(authState.currentUser?.permission_level ?? 0) >= 1
)
const validSubmissionStatusFilterValues = new Set(
  submissionStatusOptions
    .map((option) => option.value)
    .filter(Boolean)
)
const submissionLanguageFilterOptions = computed(() => {
  const options = [{ value: '', label: '전체' }]
  const seenLanguages = new Set([''])
  const candidateLanguages = [
    ...supportedSubmissionLanguages.value,
    appliedLanguageFilter.value,
    selectedLanguageFilter.value
  ]

  candidateLanguages.forEach((language) => {
    if (!language || seenLanguages.has(language)) {
      return
    }

    seenLanguages.add(language)
    options.push({
      value: language,
      label: language
    })
  })

  return options
})
const submissionCount = computed(() => submissions.value.length)
const fixedProblemId = computed(() => {
  const problemIdParam = Array.isArray(route.params.problemId)
    ? route.params.problemId[0]
    : route.params.problemId
  const parsedProblemId = Number.parseInt(problemIdParam, 10)

  return Number.isInteger(parsedProblemId) && parsedProblemId > 0
    ? parsedProblemId
    : null
})
const hasFixedProblemId = computed(() => fixedProblemId.value !== null)
const appliedProblemIdFilter = computed(() => {
  if (hasFixedProblemId.value) {
    return ''
  }

  const problemIdQuery = Array.isArray(route.query.problemId)
    ? route.query.problemId[0]
    : route.query.problemId
  const parsedProblemId = Number.parseInt(problemIdQuery, 10)

  return Number.isInteger(parsedProblemId) && parsedProblemId > 0
    ? String(parsedProblemId)
    : ''
})
const appliedStatusFilter = computed(() => {
  const routeStatus = Array.isArray(route.query.status)
    ? route.query.status[0]
    : route.query.status

  if (typeof routeStatus !== 'string') {
    return ''
  }

  return validSubmissionStatusFilterValues.has(routeStatus)
    ? routeStatus
    : ''
})
const appliedLanguageFilter = computed(() => {
  const routeLanguage = Array.isArray(route.query.language)
    ? route.query.language[0]
    : route.query.language

  return typeof routeLanguage === 'string'
    ? routeLanguage.trim()
    : ''
})
const hasAppliedStatusFilter = computed(() => Boolean(appliedStatusFilter.value))
const normalizedSelectedProblemIdFilter = computed(() => {
  if (hasFixedProblemId.value) {
    return ''
  }

  const trimmedProblemId = normalizeProblemIdFilterInputValue(selectedProblemIdFilter.value)
  if (!trimmedProblemId) {
    return ''
  }

  const parsedProblemId = Number.parseInt(trimmedProblemId, 10)
  return Number.isInteger(parsedProblemId) && parsedProblemId > 0
    ? String(parsedProblemId)
    : null
})
const hasInvalidProblemIdFilter = computed(() =>
  !hasFixedProblemId.value && normalizedSelectedProblemIdFilter.value === null
)
const canApplyFilters = computed(() =>
  !hasInvalidProblemIdFilter.value &&
  (
    selectedStatusFilter.value !== appliedStatusFilter.value ||
    selectedLanguageFilter.value !== appliedLanguageFilter.value ||
    (!hasFixedProblemId.value && normalizedSelectedProblemIdFilter.value !== appliedProblemIdFilter.value)
  )
)
const canResetFilters = computed(() =>
  Boolean(selectedStatusFilter.value) ||
  Boolean(selectedLanguageFilter.value) ||
  hasAppliedStatusFilter.value ||
  Boolean(appliedLanguageFilter.value) ||
  (!hasFixedProblemId.value && (
    Boolean(normalizeProblemIdFilterInputValue(selectedProblemIdFilter.value)) ||
    Boolean(appliedProblemIdFilter.value)
  ))
)
const pollingSubmissionIds = computed(() =>
  submissions.value
    .filter((submission) => pollingSubmissionStatuses.has(submission.status))
    .map((submission) => submission.submission_id)
)
const totalPages = computed(() =>
  Math.max(1, Math.ceil(totalSubmissionCount.value / listLimit))
)
const visibleRangeText = computed(() => {
  if (!totalSubmissionCount.value || !submissionCount.value) {
    return ''
  }

  const start = (currentPage.value - 1) * listLimit + 1
  const end = Math.min(currentPage.value * listLimit, totalSubmissionCount.value)
  return `${formatCount(start)}-${formatCount(end)} / ${formatCount(totalSubmissionCount.value)}`
})

watch(
  actionMessage,
  (message) => {
    if (message) {
      showSuccessNotice(message)
    }
  }
)

watch(
  actionErrorMessage,
  (message) => {
    if (message) {
      showErrorNotice(message, { duration: 5000 })
    }
  }
)

const paginationItems = computed(() => {
  const pages = new Set([
    1,
    totalPages.value,
    currentPage.value - 1,
    currentPage.value,
    currentPage.value + 1
  ])

  const sortedPages = Array.from(pages)
    .filter((pageNumber) => pageNumber >= 1 && pageNumber <= totalPages.value)
    .sort((left, right) => left - right)

  const items = []

  sortedPages.forEach((pageNumber, index) => {
    const previousPage = sortedPages[index - 1]

    if (index > 0) {
      if (pageNumber - previousPage === 2) {
        items.push({
          type: 'page',
          value: previousPage + 1,
          key: `page-${previousPage + 1}`
        })
      } else if (pageNumber - previousPage > 2) {
        items.push({
          type: 'ellipsis',
          key: `ellipsis-${previousPage}-${pageNumber}`
        })
      }
    }

    items.push({
      type: 'page',
      value: pageNumber,
      key: `page-${pageNumber}`
    })
  })

  return items
})
const numericProblemId = computed(() => {
  if (fixedProblemId.value !== null) {
    return fixedProblemId.value
  }

  return appliedProblemIdFilter.value
    ? Number.parseInt(appliedProblemIdFilter.value, 10)
    : null
})
const latestHistoryStatus = computed(() => {
  const latestHistoryEntry = submissionHistoryEntries.value[
    submissionHistoryEntries.value.length - 1
  ]

  return latestHistoryEntry?.to_status || ''
})
const shouldPollSubmissionHistory = computed(() =>
  historyDialogOpen.value &&
  !isLoadingHistory.value &&
  !historyErrorMessage.value &&
  Number.isInteger(activeHistorySubmissionId.value) &&
  isDocumentVisible.value &&
  Boolean(authState.token) &&
  canManageSubmissionRejudge.value &&
  pollingSubmissionStatuses.has(latestHistoryStatus.value)
)
const isMineScope = computed(() => {
  if (route.name === 'problem-my-submissions') {
    return true
  }

  if (route.name === 'problem-submissions') {
    return false
  }

  const scopeQuery = Array.isArray(route.query.scope)
    ? route.query.scope[0]
    : route.query.scope

  return scopeQuery === 'mine'
})
const numericUserId = computed(() => {
  const userIdQuery = Array.isArray(route.query.userId)
    ? route.query.userId[0]
    : route.query.userId

  const parsedUserId = Number.parseInt(userIdQuery, 10)
  return Number.isInteger(parsedUserId) && parsedUserId > 0
    ? parsedUserId
    : null
})
const activeUserId = computed(() => {
  if (isMineScope.value) {
    const currentUserId = Number(authState.currentUser?.id)
    return Number.isInteger(currentUserId) && currentUserId > 0
      ? currentUserId
      : null
  }

  return numericUserId.value
})
const pageTitle = computed(() =>
  isMineScope.value && numericProblemId.value
    ? `문제 #${formatCount(numericProblemId.value)} 내 제출`
    : isMineScope.value
      ? '내 제출'
      : numericProblemId.value
        ? `문제 #${formatCount(numericProblemId.value)} 제출 목록`
        : '제출 목록'
)
const copyButtonLabel = computed(() => {
  if (copyState.value === 'success') {
    return '복사됨'
  }

  if (copyState.value === 'error') {
    return '복사 실패'
  }

  return '복사'
})

const statusLabelMap = {
  queued: '대기 중',
  judging: '채점 중',
  accepted: '정답',
  wrong_answer: '오답',
  time_limit_exceeded: '시간 초과',
  memory_limit_exceeded: '메모리 초과',
  runtime_error: '런타임 에러',
  output_exceeded: '출력 초과',
  compile_error: '컴파일 에러'
}

const statusToneMap = {
  queued: 'neutral',
  judging: 'warning',
  accepted: 'success',
  wrong_answer: 'danger',
  time_limit_exceeded: 'danger',
  memory_limit_exceeded: 'danger',
  runtime_error: 'danger',
  output_exceeded: 'danger',
  compile_error: 'danger'
}

const finishedSubmissionStatuses = new Set([
  'accepted',
  'wrong_answer',
  'time_limit_exceeded',
  'memory_limit_exceeded',
  'runtime_error',
  'output_exceeded',
  'compile_error'
])

let copyStateResetTimer = null
let latestLoadRequestId = 0
let latestHistoryLoadRequestId = 0
let relativeTimeRefreshTimer = null
let submissionPollingTimer = null
let isPollingSubmissionDetails = false
let historyPollingTimer = null
let isPollingSubmissionHistory = false

function formatCount(value){
  return countFormatter.format(value)
}

function normalizeProblemIdFilterInputValue(value){
  if (value === null || typeof value === 'undefined') {
    return ''
  }

  return String(value).trim()
}

function normalizeSubmissionMetric(value){
  if (value === null || typeof value === 'undefined' || value === '') {
    return null
  }

  const parsedValue = Number(value)
  return Number.isFinite(parsedValue) ? parsedValue : null
}

function resetPagination(){
  currentPage.value = 1
  pageJumpInput.value = ''
  totalSubmissionCount.value = 0
}

async function loadSupportedSubmissionLanguages(){
  isLoadingLanguages.value = true

  try {
    const response = await getSupportedLanguages()
    const languages = Array.isArray(response.languages) ? response.languages : []

    supportedSubmissionLanguages.value = languages
      .map((language) => typeof language?.language === 'string' ? language.language.trim() : '')
      .filter((language, index, values) => language && values.indexOf(language) === index)
  } catch {
    supportedSubmissionLanguages.value = []
  } finally {
    isLoadingLanguages.value = false
  }
}

function makeSubmissionFilterQuery(problemId, status, language){
  const nextQuery = {
    ...route.query
  }

  if (!hasFixedProblemId.value) {
    if (problemId) {
      nextQuery.problemId = problemId
    } else {
      delete nextQuery.problemId
    }
  }

  if (status) {
    nextQuery.status = status
  } else {
    delete nextQuery.status
  }

  if (language) {
    nextQuery.language = language
  } else {
    delete nextQuery.language
  }

  return nextQuery
}

async function applySubmissionFilters(){
  if (isLoading.value || !canApplyFilters.value) {
    return
  }

  await router.replace({
    name: route.name,
    params: route.params,
    query: makeSubmissionFilterQuery(
      normalizedSelectedProblemIdFilter.value || '',
      selectedStatusFilter.value,
      selectedLanguageFilter.value
    )
  })
}

async function resetSubmissionFilters(){
  selectedProblemIdFilter.value = ''
  selectedStatusFilter.value = ''
  selectedLanguageFilter.value = ''

  if (isLoading.value || !canResetFilters.value) {
    return
  }

  await router.replace({
    name: route.name,
    params: route.params,
    query: makeSubmissionFilterQuery('', '', '')
  })
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

function formatRelativeSubmittedAt(timestamp){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return '-'
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

function formatElapsedMs(value){
  if (typeof value !== 'number') {
    return '-'
  }

  return `${countFormatter.format(value)} ms`
}

function formatMemory(value){
  if (typeof value !== 'number') {
    return '-'
  }

  return `${countFormatter.format(value)} KB`
}

function normalizeSubmissionHistoryEntry(historyEntry, index){
  const normalizedCreatedAt = normalizeSubmittedAt(historyEntry?.created_at)
  const numericHistoryId = Number(historyEntry?.history_id)
  const fromStatus = typeof historyEntry?.from_status === 'string' && historyEntry.from_status
    ? historyEntry.from_status
    : null
  const toStatus = typeof historyEntry?.to_status === 'string' && historyEntry.to_status
    ? historyEntry.to_status
    : 'queued'
  const createdAtLabel = normalizedCreatedAt.label ||
    (typeof historyEntry?.created_at === 'string' && historyEntry.created_at.trim()
      ? historyEntry.created_at.trim()
      : '-')

  return {
    history_id: Number.isInteger(numericHistoryId) && numericHistoryId > 0
      ? numericHistoryId
      : null,
    history_key: Number.isInteger(numericHistoryId) && numericHistoryId > 0
      ? `history-${numericHistoryId}`
      : `history-fallback-${index}-${createdAtLabel}`,
    from_status: fromStatus,
    to_status: toStatus,
    reason: typeof historyEntry?.reason === 'string' ? historyEntry.reason.trim() : '',
    created_at_label: createdAtLabel
  }
}

function formatHistoryTransition(historyEntry){
  if (!historyEntry?.from_status) {
    return 'queued'
  }

  return `${historyEntry.from_status} -> ${historyEntry.to_status}`
}

function getStatusLabel(status){
  return statusLabelMap[status] || status
}

function getStatusTone(status){
  return statusToneMap[status] || 'neutral'
}

function canRejudgeSubmission(submission){
  if (!canManageSubmissionRejudge.value || !authState.token) {
    return false
  }

  return finishedSubmissionStatuses.has(submission.status)
}

function stopHistoryPolling(){
  if (historyPollingTimer) {
    clearTimeout(historyPollingTimer)
    historyPollingTimer = null
  }
}

function startHistoryPolling(){
  if (
    typeof window === 'undefined' ||
    historyPollingTimer ||
    isPollingSubmissionHistory ||
    !shouldPollSubmissionHistory.value
  ) {
    return
  }

  historyPollingTimer = window.setTimeout(() => {
    historyPollingTimer = null
    pollSubmissionHistory()
  }, submissionPollingIntervalMs)
}

function syncHistoryPolling(){
  if (!shouldPollSubmissionHistory.value) {
    stopHistoryPolling()
    return
  }

  startHistoryPolling()
}

function isRejudgingSubmission(submissionId){
  return rejudgingSubmissionIds.value.includes(submissionId)
}

function addRejudgingSubmission(submissionId){
  if (isRejudgingSubmission(submissionId)) {
    return
  }

  rejudgingSubmissionIds.value = [...rejudgingSubmissionIds.value, submissionId]
}

function removeRejudgingSubmission(submissionId){
  rejudgingSubmissionIds.value = rejudgingSubmissionIds.value.filter(
    (queuedSubmissionId) => queuedSubmissionId !== submissionId
  )
}

function patchSubmission(submissionId, patch){
  submissions.value = submissions.value.map((submission) =>
    submission.submission_id === submissionId
      ? {
        ...submission,
        ...patch
      }
      : submission
  )
}

function shouldPollSubmissions(){
  return !isLoading.value &&
    !errorMessage.value &&
    isDocumentVisible.value &&
    !hasAppliedStatusFilter.value &&
    pollingSubmissionIds.value.length > 0
}

function stopSubmissionPolling(){
  if (submissionPollingTimer) {
    clearTimeout(submissionPollingTimer)
    submissionPollingTimer = null
  }
}

function startSubmissionPolling(){
  if (typeof window === 'undefined' || submissionPollingTimer || isPollingSubmissionDetails || !shouldPollSubmissions()) {
    return
  }

  submissionPollingTimer = window.setTimeout(() => {
    submissionPollingTimer = null
    pollActiveSubmissions()
  }, submissionPollingIntervalMs)
}

function syncSubmissionPolling(){
  if (!shouldPollSubmissions()) {
    stopSubmissionPolling()
    return
  }

  startSubmissionPolling()
}

function updateSubmissionFromDetail(detail){
  const submissionId = Number(detail?.submission_id)

  if (!Number.isInteger(submissionId) || submissionId <= 0) {
    return
  }

  submissions.value = submissions.value.map((submission) =>
    submission.submission_id === submissionId
      ? {
        ...submission,
        status: typeof detail.status === 'string' && detail.status
          ? detail.status
          : submission.status,
        elapsed_ms: normalizeSubmissionMetric(detail.elapsed_ms),
        max_rss_kb: normalizeSubmissionMetric(detail.max_rss_kb)
      }
      : submission
  )
}

async function pollActiveSubmissions(){
  if (isPollingSubmissionDetails || !shouldPollSubmissions()) {
    return
  }

  isPollingSubmissionDetails = true
  const activeSubmissionIds = [...new Set(pollingSubmissionIds.value)]

  try {
    const results = await Promise.allSettled(
      activeSubmissionIds.map((submissionId) =>
        getSubmissionDetail(submissionId, {
          bearerToken: authenticatedBearerToken.value
        })
      )
    )

    results.forEach((result) => {
      if (result.status === 'fulfilled') {
        updateSubmissionFromDetail(result.value)
      }
    })
  } finally {
    isPollingSubmissionDetails = false
    syncSubmissionPolling()
  }
}

function handleDocumentVisibilityChange(){
  if (typeof document === 'undefined') {
    isDocumentVisible.value = true
  } else {
    isDocumentVisible.value = !document.hidden
  }

  syncSubmissionPolling()
}

function canViewSource(submission){
  if (!isAuthenticated.value || !authState.currentUser) {
    return false
  }

  if (Number(authState.currentUser.permission_level ?? 0) >= 1) {
    return true
  }

  return Number(submission.user_id) === Number(authState.currentUser.id)
}

function resetCopyState(){
  copyState.value = 'idle'

  if (copyStateResetTimer) {
    clearTimeout(copyStateResetTimer)
    copyStateResetTimer = null
  }
}

function scheduleCopyStateReset(){
  if (copyStateResetTimer) {
    clearTimeout(copyStateResetTimer)
  }

  copyStateResetTimer = window.setTimeout(() => {
    copyState.value = 'idle'
    copyStateResetTimer = null
  }, 1400)
}

function fallbackCopyText(text){
  if (typeof document === 'undefined') {
    throw new Error('clipboard unavailable')
  }

  const helperTextArea = document.createElement('textarea')
  helperTextArea.value = text
  helperTextArea.setAttribute('readonly', '')
  helperTextArea.style.position = 'fixed'
  helperTextArea.style.opacity = '0'
  helperTextArea.style.pointerEvents = 'none'

  document.body.appendChild(helperTextArea)
  helperTextArea.select()
  helperTextArea.setSelectionRange(0, helperTextArea.value.length)

  const copySucceeded = document.execCommand('copy')
  document.body.removeChild(helperTextArea)

  if (!copySucceeded) {
    throw new Error('clipboard unavailable')
  }
}

async function handleRejudgeSubmission(submission){
  if (!authState.token || !canRejudgeSubmission(submission) || isRejudgingSubmission(submission.submission_id)) {
    return
  }

  const submissionId = submission.submission_id
  addRejudgingSubmission(submissionId)
  actionMessage.value = ''
  actionErrorMessage.value = ''

  try {
    const response = await rejudgeSubmission(submissionId, authState.token)
    patchSubmission(submissionId, {
      status: response.status || 'queued',
      elapsed_ms: null,
      max_rss_kb: null
    })
    actionMessage.value = `제출 #${formatCount(submissionId)} 재채점을 요청했습니다.`
  } catch (error) {
    actionErrorMessage.value = error instanceof Error
      ? error.message
      : '제출 재채점을 요청하지 못했습니다.'
  } finally {
    removeRejudgingSubmission(submissionId)
  }
}

function closeHistoryDialog(){
  stopHistoryPolling()
  latestHistoryLoadRequestId += 1
  isHistoryBackdropInteraction.value = false
  historyDialogOpen.value = false
  isLoadingHistory.value = false
  historyErrorMessage.value = ''
  submissionHistoryEntries.value = []
  activeHistorySubmissionId.value = null
}

function handleHistoryBackdropPointerDown(event){
  isHistoryBackdropInteraction.value = event.target === event.currentTarget
}

function handleHistoryBackdropClick(){
  if (!isHistoryBackdropInteraction.value) {
    return
  }

  closeHistoryDialog()
}

async function fetchSubmissionHistory(submissionId, options = {}){
  if (!authState.token || !canManageSubmissionRejudge.value) {
    return
  }

  const { background = false } = options
  const requestId = ++latestHistoryLoadRequestId

  if (!background) {
    isLoadingHistory.value = true
    historyErrorMessage.value = ''
    submissionHistoryEntries.value = []
  }

  try {
    const response = await getSubmissionHistory(submissionId, authState.token)

    if (
      requestId !== latestHistoryLoadRequestId ||
      activeHistorySubmissionId.value !== submissionId
    ) {
      return
    }

    const normalizedHistories = Array.isArray(response.histories)
      ? response.histories.map((historyEntry, index) =>
        normalizeSubmissionHistoryEntry(historyEntry, index)
      )
      : []

    submissionHistoryEntries.value = normalizedHistories

    const latestHistoryEntry = normalizedHistories[normalizedHistories.length - 1]
    if (latestHistoryEntry) {
      patchSubmission(submissionId, {
        status: latestHistoryEntry.to_status
      })
    }
  } catch (error) {
    if (
      requestId !== latestHistoryLoadRequestId ||
      activeHistorySubmissionId.value !== submissionId
    ) {
      return
    }

    if (!background) {
      historyErrorMessage.value = error instanceof Error
        ? error.message
        : '채점 내역을 불러오지 못했습니다.'
      submissionHistoryEntries.value = []
    }
  } finally {
    if (
      !background &&
      requestId === latestHistoryLoadRequestId &&
      activeHistorySubmissionId.value === submissionId
    ) {
      isLoadingHistory.value = false
    }
  }
}

async function openHistoryDialog(submission){
  if (!authState.token || !canManageSubmissionRejudge.value) {
    return
  }

  historyDialogOpen.value = true
  isLoadingHistory.value = true
  historyErrorMessage.value = ''
  submissionHistoryEntries.value = []
  activeHistorySubmissionId.value = submission.submission_id

  await fetchSubmissionHistory(submission.submission_id)
}

async function pollSubmissionHistory(){
  if (
    isPollingSubmissionHistory ||
    !shouldPollSubmissionHistory.value ||
    !Number.isInteger(activeHistorySubmissionId.value)
  ) {
    return
  }

  isPollingSubmissionHistory = true

  try {
    await fetchSubmissionHistory(activeHistorySubmissionId.value, {
      background: true
    })
  } finally {
    isPollingSubmissionHistory = false
    syncHistoryPolling()
  }
}

function closeSourceDialog(){
  isSourceBackdropInteraction.value = false
  sourceDialogOpen.value = false
  isLoadingSource.value = false
  sourceErrorMessage.value = ''
  sourceDetail.value = null
  activeSourceSubmissionId.value = null
  resetCopyState()
}

function handleSourceBackdropPointerDown(event){
  isSourceBackdropInteraction.value = event.target === event.currentTarget
}

function handleSourceBackdropClick(){
  if (!isSourceBackdropInteraction.value) {
    return
  }

  closeSourceDialog()
}

async function openSourceDialog(submission){
  if (!canViewSource(submission) || !authState.token) {
    return
  }

  sourceDialogOpen.value = true
  isLoadingSource.value = true
  sourceErrorMessage.value = ''
  sourceDetail.value = null
  activeSourceSubmissionId.value = submission.submission_id

  try {
    const response = await getSubmissionSource(submission.submission_id, authState.token)
    sourceDetail.value = {
      submission_id: Number(response.submission_id),
      language: response.language || submission.language,
      source_code: response.source_code || '',
      compile_output: typeof response.compile_output === 'string' ? response.compile_output : '',
      judge_output: typeof response.judge_output === 'string' ? response.judge_output : ''
    }
  } catch (error) {
    sourceErrorMessage.value = error instanceof Error
      ? error.message
      : '소스 코드를 불러오지 못했습니다.'
  } finally {
    isLoadingSource.value = false
  }
}

async function copySourceCode(){
  if (!sourceDetail.value?.source_code) {
    return
  }

  try {
    if (typeof navigator !== 'undefined' && navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(sourceDetail.value.source_code)
    } else {
      fallbackCopyText(sourceDetail.value.source_code)
    }

    copyState.value = 'success'
  } catch {
    copyState.value = 'error'
  }

  scheduleCopyStateReset()
}

function stopRelativeTimeRefresh(){
  if (relativeTimeRefreshTimer) {
    clearInterval(relativeTimeRefreshTimer)
    relativeTimeRefreshTimer = null
  }
}

function startRelativeTimeRefresh(){
  stopRelativeTimeRefresh()
  nowTimestamp.value = Date.now()
  relativeTimeRefreshTimer = window.setInterval(() => {
    nowTimestamp.value = Date.now()
  }, 1000)
}

async function loadSubmissions(options = {}){
  const targetPageNumber = Number.isInteger(options.pageNumber) && options.pageNumber > 0
    ? options.pageNumber
    : currentPage.value
  const requestId = ++latestLoadRequestId
  isLoading.value = true
  errorMessage.value = ''
  rejudgingSubmissionIds.value = []

  if (isMineScope.value && !isAuthenticated.value) {
    submissions.value = []
    errorMessage.value = '내 제출을 보려면 로그인하세요.'
    if (requestId === latestLoadRequestId) {
      isLoading.value = false
    }
    hasLoadedOnce.value = true
    return
  }

  try {
    const response = await getSubmissionList({
      limit: listLimit,
      page: targetPageNumber,
      problemId: numericProblemId.value ?? undefined,
      userId: activeUserId.value ?? undefined,
      language: appliedLanguageFilter.value || undefined,
      status: appliedStatusFilter.value || undefined,
      bearerToken: authenticatedBearerToken.value
    })

    if (requestId !== latestLoadRequestId) {
      return
    }

    const normalizedSubmissions = Array.isArray(response.submissions)
      ? response.submissions
        .map((submission) => {
          const normalizedSubmittedAt = normalizeSubmittedAt(submission.created_at)

          return {
            ...submission,
            submission_id: Number(submission.submission_id),
            user_id: Number(submission.user_id),
            problem_id: Number(submission.problem_id),
            user_name: submission.user_name || `사용자 ${countFormatter.format(submission.user_id)}`,
            created_at: typeof submission.created_at === 'string' ? submission.created_at : '',
            created_at_timestamp: normalizedSubmittedAt.timestamp,
            created_at_label: normalizedSubmittedAt.label,
            elapsed_ms: typeof submission.elapsed_ms === 'number' ? submission.elapsed_ms : null,
            max_rss_kb: typeof submission.max_rss_kb === 'number' ? submission.max_rss_kb : null
          }
        })
        .sort((left, right) => right.submission_id - left.submission_id)
      : []

    const normalizedTotalSubmissionCount = Number(
      response.total_submission_count ?? response.submission_count ?? normalizedSubmissions.length
    )
    const nextTotalPages =
      normalizedTotalSubmissionCount > 0
        ? Math.max(1, Math.ceil(normalizedTotalSubmissionCount / listLimit))
        : 1

    if (normalizedTotalSubmissionCount > 0 && targetPageNumber > nextTotalPages) {
      currentPage.value = nextTotalPages
      loadSubmissions({
        pageNumber: nextTotalPages
      })
      return
    }

    submissions.value = normalizedSubmissions
    totalSubmissionCount.value = normalizedTotalSubmissionCount
    currentPage.value = normalizedTotalSubmissionCount > 0 ? targetPageNumber : 1
    hasLoadedOnce.value = true
  } catch (error) {
    if (requestId !== latestLoadRequestId) {
      return
    }

    errorMessage.value = error instanceof Error
      ? error.message
      : '제출 목록을 불러오지 못했습니다.'
    submissions.value = []
    totalSubmissionCount.value = 0
    hasLoadedOnce.value = true
  } finally {
    if (requestId === latestLoadRequestId) {
      isLoading.value = false
    }
  }
}

function goToPage(pageNumber){
  if (isLoading.value) {
    return
  }

  const clampedPageNumber = Math.min(Math.max(pageNumber, 1), totalPages.value)
  if (clampedPageNumber === currentPage.value) {
    return
  }

  loadSubmissions({
    pageNumber: clampedPageNumber
  })
}

function submitPageJump(){
  const parsedPage = Number.parseInt(pageJumpInput.value, 10)

  if (Number.isNaN(parsedPage)) {
    return
  }

  goToPage(parsedPage)
  pageJumpInput.value = ''
}

onMounted(async () => {
  startRelativeTimeRefresh()
  handleDocumentVisibilityChange()
  selectedProblemIdFilter.value = appliedProblemIdFilter.value
  selectedStatusFilter.value = appliedStatusFilter.value
  selectedLanguageFilter.value = appliedLanguageFilter.value

  if (typeof document !== 'undefined') {
    document.addEventListener('visibilitychange', handleDocumentVisibilityChange)
  }

  loadSupportedSubmissionLanguages()

  if (!authState.initialized) {
    await initializeAuth()
  }

  if (!hasLoadedOnce.value) {
    loadSubmissions()
  }
})

watch([
  () => route.name,
  numericProblemId,
  numericUserId,
  isMineScope,
  appliedLanguageFilter,
  appliedStatusFilter
], () => {
  if (!authState.initialized) {
    return
  }

  resetPagination()
  loadSubmissions()
})

watch(
  appliedProblemIdFilter,
  (problemId) => {
    if (!hasFixedProblemId.value) {
      selectedProblemIdFilter.value = problemId
    }
  },
  { immediate: true }
)

watch(
  appliedStatusFilter,
  (status) => {
    selectedStatusFilter.value = status
  },
  { immediate: true }
)

watch(
  appliedLanguageFilter,
  (language) => {
    selectedLanguageFilter.value = language
  },
  { immediate: true }
)

watch(authenticatedBearerToken, (nextToken, previousToken) => {
  if (!authState.initialized || nextToken === previousToken) {
    return
  }

  if (isMineScope.value) {
    resetPagination()
  }

  loadSubmissions()
})

watch(totalPages, (pageCount) => {
  if (currentPage.value > pageCount) {
    currentPage.value = pageCount
  }
})

watch(
  [pollingSubmissionIds, isDocumentVisible, isLoading, errorMessage],
  () => {
    syncSubmissionPolling()
  },
  { immediate: true }
)

watch(
  [
    shouldPollSubmissionHistory,
    latestHistoryStatus,
    activeHistorySubmissionId
  ],
  () => {
    syncHistoryPolling()
  },
  { immediate: true }
)

onUnmounted(() => {
  stopHistoryPolling()
  stopSubmissionPolling()
  stopRelativeTimeRefresh()
  resetCopyState()

  if (typeof document !== 'undefined') {
    document.removeEventListener('visibilitychange', handleDocumentVisibilityChange)
  }
})
</script>

<style scoped>
.submissions-panel {
  display: grid;
  gap: 1.25rem;
}

.submissions-feedback {
  padding: 1rem;
  border-radius: 18px;
  border: 1px solid transparent;
  background: var(--surface-strong);
}

.submissions-feedback p {
  margin: 0;
}

.submissions-feedback.is-success {
  color: var(--success);
  background: var(--success-soft);
  border-color: rgba(15, 118, 110, 0.16);
}

.submissions-feedback.is-error {
  color: var(--danger);
  background: var(--danger-soft);
  border-color: rgba(185, 28, 28, 0.18);
}

.submissions-back-link {
  display: inline-flex;
  align-items: center;
  min-height: 2.6rem;
  padding: 0.6rem 1rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.72);
  font-weight: 700;
  transition:
    transform 160ms ease,
    background 160ms ease,
    border-color 160ms ease;
}

.submissions-back-link:hover {
  transform: translateY(-1px);
  border-color: rgba(20, 33, 61, 0.24);
}

.submissions-toolbar {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 1rem;
  flex-wrap: wrap;
}

.submissions-toolbar-actions {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 0.75rem;
  flex-wrap: wrap;
  margin-left: auto;
}

.submission-filter-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 1rem;
  flex-wrap: wrap;
  padding-inline: 1.25rem;
}

.submission-filter-fields {
  display: flex;
  align-items: center;
  gap: 1rem;
  flex-wrap: wrap;
}

.submission-summary-group {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.submission-filter-group,
.submission-filter-actions {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.submission-filter-label {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 700;
}

.submission-filter-select {
  min-width: 12rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.95rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.9);
  color: var(--ink-strong);
  font: inherit;
}

.submission-filter-input {
  width: 9rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.95rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.9);
  color: var(--ink-strong);
  font: inherit;
}

.submission-filter-input:focus,
.submission-filter-select:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.submission-summary-text {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 600;
}

.submission-table-wrapper {
  overflow-x: auto;
}

.submission-table {
  min-width: 1020px;
  border: 1px solid var(--line);
  border-radius: 1.25rem;
  overflow: visible;
  background: rgba(255, 255, 255, 0.76);
}

.submission-table.has-actions {
  min-width: 1280px;
}

.submission-table-head,
.submission-row {
  display: grid;
  grid-template-columns:
    minmax(88px, 0.9fr)
    minmax(140px, 1.2fr)
    minmax(96px, 0.95fr)
    minmax(120px, 1.1fr)
    minmax(104px, 0.95fr)
    minmax(112px, 1fr)
    minmax(96px, 0.9fr)
    minmax(124px, 1.1fr);
  column-gap: 1rem;
  align-items: center;
  padding: 0.95rem 1.25rem;
}

.submission-table-head.has-actions,
.submission-row.has-actions {
  grid-template-columns:
    minmax(88px, 0.9fr)
    minmax(140px, 1.2fr)
    minmax(96px, 0.95fr)
    minmax(120px, 1.1fr)
    minmax(104px, 0.95fr)
    minmax(112px, 1fr)
    minmax(96px, 0.9fr)
    minmax(124px, 1.1fr)
    minmax(112px, 0.95fr)
    minmax(112px, 0.95fr);
}

.submission-table-head {
  background: rgba(20, 33, 61, 0.06);
  border-bottom: 1px solid var(--line);
  font-size: 0.82rem;
  font-weight: 700;
  color: var(--ink-soft);
}

.submission-table-head-user {
  margin-left: -1.5rem;
}

.submission-row + .submission-row {
  border-top: 1px solid rgba(20, 33, 61, 0.08);
}

.submission-row:hover {
  background: rgba(255, 255, 255, 0.94);
}

.submission-cell {
  min-width: 0;
  color: var(--ink-strong);
  font-variant-numeric: tabular-nums;
}

.submission-cell.is-submitted-at {
  white-space: nowrap;
  font-size: 0.9rem;
  font-weight: 600;
  color: #6C8BCF;
}

.submission-relative-time {
  position: relative;
  display: inline-flex;
  align-items: center;
  color: inherit;
  cursor: help;
  outline: none;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 140ms ease,
    text-decoration-color 140ms ease,
    transform 140ms ease;
}

.submission-relative-time:hover,
.submission-relative-time:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-time-tooltip {
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

.submission-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.submission-relative-time:hover .submission-time-tooltip,
.submission-relative-time:focus-visible .submission-time-tooltip {
  opacity: 1;
  transform: translateY(0);
}

.submission-cell.is-number,
.submission-cell.is-user,
.submission-cell.is-metric,
.submission-cell.is-language {
  font-weight: 600;
}

.submission-action-cell {
  display: flex;
  justify-content: center;
}

.submission-head-action {
  justify-self: center;
}

.submission-cell.is-user {
  margin-left: -1.5rem;
}

.submission-problem-link {
  min-width: 0;
  font-weight: 700;
  color: #6C8BCF;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 160ms ease,
    text-decoration-color 160ms ease,
    transform 160ms ease;
}

.submission-problem-link:hover,
.submission-problem-link:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-problem-link.problem-state-text--solved {
  color: var(--success);
}

.submission-problem-link.problem-state-text--wrong {
  color: var(--danger);
}

.submission-problem-link.problem-state-text--solved:hover,
.submission-problem-link.problem-state-text--solved:focus-visible {
  color: var(--success);
  text-decoration-color: currentColor;
}

.submission-problem-link.problem-state-text--wrong:hover,
.submission-problem-link.problem-state-text--wrong:focus-visible {
  color: var(--danger);
  text-decoration-color: currentColor;
}

.submission-language-button {
  display: inline-flex;
  align-items: center;
  justify-content: flex-start;
  min-width: 0;
  padding: 0;
  border: 0;
  background: transparent;
  color: var(--accent);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
  text-align: left;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 160ms ease,
    text-decoration-color 160ms ease,
    transform 160ms ease;
}

.submission-language-button:hover,
.submission-language-button:focus-visible {
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-rejudge-button {
  min-width: 6rem;
}

.submission-history-button {
  min-width: 6rem;
}

.submission-cell.is-language {
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 160ms ease,
    text-decoration-color 160ms ease,
    transform 160ms ease;
}

.submission-cell.is-language:hover {
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.submission-pagination {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
  padding-top: 0.25rem;
}

.pagination-controls {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.pagination-button {
  min-width: 88px;
}

.pagination-pages {
  display: flex;
  gap: 0.55rem;
  flex-wrap: wrap;
  justify-content: center;
}

.pagination-jump {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  margin-left: auto;
}

.pagination-jump-input {
  width: 7rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.9rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.pagination-jump-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.pagination-page {
  min-width: 2.75rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.9rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.72);
  color: var(--ink-strong);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 160ms ease,
    background 160ms ease,
    border-color 160ms ease;
}

.pagination-page:hover {
  transform: translateY(-1px);
}

.pagination-page.is-active {
  color: white;
  background: linear-gradient(135deg, #d97706, #ea580c);
  border-color: transparent;
}

.pagination-ellipsis {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 2.75rem;
  min-height: 2.75rem;
  color: var(--ink-soft);
  font-weight: 700;
}

.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}

.submission-history-backdrop {
  position: fixed;
  inset: 0;
  z-index: 41;
  display: grid;
  place-items: center;
  padding: 1.5rem;
  background: rgba(15, 23, 42, 0.42);
  backdrop-filter: blur(10px);
}

.submission-history-dialog {
  width: min(760px, 100%);
  max-height: calc(100vh - 3rem);
  overflow: auto;
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.95);
  box-shadow: var(--shadow);
  padding: 1.4rem;
  display: grid;
  gap: 1rem;
}

.submission-history-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 1rem;
}

.submission-history-live {
  margin: 0;
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 700;
}

.submission-history-timeline {
  position: relative;
  display: grid;
  gap: 0.95rem;
  padding-left: 0.25rem;
}

.submission-history-timeline::before {
  content: '';
  position: absolute;
  left: 0.7rem;
  top: 0.5rem;
  bottom: 0.5rem;
  width: 1px;
  background: rgba(20, 33, 61, 0.12);
}

.submission-history-entry {
  position: relative;
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 0.9rem;
  align-items: start;
}

.submission-history-entry-marker {
  position: relative;
  z-index: 1;
  width: 1.4rem;
  height: 1.4rem;
  margin-top: 0.7rem;
  border-radius: 999px;
  border: 4px solid rgba(217, 119, 6, 0.18);
  background: white;
  box-shadow: 0 0 0 4px rgba(255, 255, 255, 0.95);
}

.submission-history-entry.is-current .submission-history-entry-marker {
  border-color: rgba(217, 119, 6, 0.32);
}

.submission-history-entry-card {
  display: grid;
  gap: 0.65rem;
  padding: 1rem 1.05rem;
  border: 1px solid rgba(20, 33, 61, 0.08);
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.88);
}

.submission-history-entry.is-current .submission-history-entry-card {
  border-color: rgba(217, 119, 6, 0.22);
  background: rgba(255, 247, 237, 0.9);
}

.submission-history-entry-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.submission-history-entry-time {
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 700;
  white-space: nowrap;
}

.submission-history-entry-transition {
  margin: 0;
  color: var(--ink-soft);
  font-size: 0.9rem;
  font-weight: 700;
}

.submission-history-entry-reason {
  margin: 0;
  padding: 0.8rem 0.9rem;
  border-radius: 16px;
  background: rgba(20, 33, 61, 0.05);
  color: var(--ink-strong);
  font-size: 0.92rem;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

.submission-history-empty-state {
  margin-top: 0.25rem;
}

.submission-source-backdrop {
  position: fixed;
  inset: 0;
  z-index: 40;
  display: grid;
  place-items: center;
  padding: 1.5rem;
  background: rgba(15, 23, 42, 0.42);
  backdrop-filter: blur(10px);
}

.submission-source-dialog {
  width: min(960px, 100%);
  max-height: calc(100vh - 3rem);
  overflow: auto;
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.95);
  box-shadow: var(--shadow);
  padding: 1.4rem;
}

.submission-source-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 1rem;
}

.submission-source-content {
  display: grid;
  gap: 1rem;
}

.submission-source-meta {
  display: flex;
  justify-content: flex-start;
}

.submission-source-diagnostics {
  display: grid;
  gap: 0.9rem;
}

.submission-source-diagnostic {
  display: grid;
  gap: 0.5rem;
}

.submission-source-diagnostic-title {
  margin: 0;
  color: var(--ink-soft);
  font-size: 0.9rem;
  font-weight: 700;
}

.submission-source-diagnostic-log {
  margin: 0;
  padding: 1rem 1.1rem;
  border-radius: 18px;
  border: 1px solid rgba(185, 28, 28, 0.14);
  background: rgba(185, 28, 28, 0.06);
  overflow: auto;
  color: var(--danger);
  font-family: "SFMono-Regular", "Consolas", monospace;
  font-size: 0.9rem;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

.submission-source-actions {
  display: flex;
  justify-content: flex-end;
}

.submission-source-code {
  margin: 0;
  padding: 1rem 1.1rem;
  border-radius: 18px;
  background: rgba(20, 33, 61, 0.06);
  overflow: auto;
  color: var(--ink-strong);
  font-family: "SFMono-Regular", "Consolas", monospace;
  font-size: 0.92rem;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

@media (max-width: 720px) {
  .submissions-toolbar,
  .submissions-toolbar-actions,
  .submission-filter-fields,
  .submission-filter-bar,
  .submission-pagination {
    flex-direction: column;
    align-items: stretch;
  }

  .pagination-controls,
  .pagination-jump {
    justify-content: center;
    margin-left: 0;
  }

  .pagination-jump-input {
    width: 100%;
  }

  .submission-table {
    min-width: 980px;
  }

  .submission-table.has-actions {
    min-width: 1240px;
  }

  .submission-history-dialog,
  .submission-source-dialog {
    width: 100%;
  }

  .submission-history-entry-top {
    align-items: flex-start;
  }
}
</style>
