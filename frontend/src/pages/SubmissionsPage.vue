<template>
  <section class="page-grid single-column">
    <article class="panel submissions-panel">
      <div
        v-if="numericProblemId"
        class="submissions-header"
      >
        <RouterLink
          class="submissions-back-link"
          :to="{ name: 'problem-detail', params: { problemId: numericProblemId } }"
        >
          문제로 돌아가기
        </RouterLink>
      </div>

      <div class="submissions-toolbar">
        <div>
          <p class="panel-kicker">submissions</p>
          <h3>{{ pageTitle }}</h3>
        </div>
      </div>

      <div class="submission-summary-bar">
        <StatusBadge
          :label="isLoading ? '불러오는 중' : `${submissionCount}개 제출`"
          :tone="errorMessage ? 'danger' : 'neutral'"
        />
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>제출 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else-if="!submissionCount" class="empty-state">
        <p>등록된 제출이 아직 없습니다.</p>
      </div>

      <div v-else class="submission-table-wrapper">
        <div class="submission-table">
          <div class="submission-table-head">
            <span>제출번호</span>
            <span class="submission-table-head-user">닉네임</span>
            <span>문제번호</span>
            <span>결과</span>
            <span>실행시간</span>
            <span>메모리</span>
            <span>언어</span>
            <span>제출 시각</span>
          </div>

          <div
            v-for="submission in submissions"
            :key="submission.submission_id"
            class="submission-row"
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
          </div>
        </div>
      </div>
    </article>
  </section>

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
import { useRoute } from 'vue-router'

import { getSubmissionList, getSubmissionSource } from '@/api/submission'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'
import { getProblemStateTextClass } from '@/utils/problemState'

const route = useRoute()
const { authState, isAuthenticated, initializeAuth } = useAuth()
const listLimit = 50
const isLoading = ref(true)
const errorMessage = ref('')
const submissions = ref([])
const hasLoadedOnce = ref(false)
const sourceDialogOpen = ref(false)
const isLoadingSource = ref(false)
const sourceErrorMessage = ref('')
const sourceDetail = ref(null)
const activeSourceSubmissionId = ref(null)
const copyState = ref('idle')
const isSourceBackdropInteraction = ref(false)
const nowTimestamp = ref(Date.now())
const countFormatter = new Intl.NumberFormat()
const authenticatedBearerToken = computed(() =>
  authState.initialized && isAuthenticated.value ? authState.token : ''
)
const submissionCount = computed(() => submissions.value.length)
const numericProblemId = computed(() => {
  const problemIdParam = Array.isArray(route.params.problemId)
    ? route.params.problemId[0]
    : route.params.problemId
  const problemIdQuery = Array.isArray(route.query.problemId)
    ? route.query.problemId[0]
    : route.query.problemId
  const sourceValue = problemIdParam || problemIdQuery

  const parsedProblemId = Number.parseInt(sourceValue, 10)
  return Number.isInteger(parsedProblemId) && parsedProblemId > 0
    ? parsedProblemId
    : null
})
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
  compile_error: 'danger'
}

let copyStateResetTimer = null
let latestLoadRequestId = 0
let relativeTimeRefreshTimer = null

function formatCount(value){
  return countFormatter.format(value)
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

function getStatusLabel(status){
  return statusLabelMap[status] || status
}

function getStatusTone(status){
  return statusToneMap[status] || 'neutral'
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
      source_code: response.source_code || ''
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

async function loadSubmissions(){
  const requestId = ++latestLoadRequestId
  isLoading.value = true
  errorMessage.value = ''

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
      problemId: numericProblemId.value ?? undefined,
      userId: activeUserId.value ?? undefined,
      bearerToken: authenticatedBearerToken.value
    })

    if (requestId !== latestLoadRequestId) {
      return
    }

    submissions.value = Array.isArray(response.submissions)
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
    hasLoadedOnce.value = true
  } catch (error) {
    if (requestId !== latestLoadRequestId) {
      return
    }

    errorMessage.value = error instanceof Error
      ? error.message
      : '제출 목록을 불러오지 못했습니다.'
    submissions.value = []
    hasLoadedOnce.value = true
  } finally {
    if (requestId === latestLoadRequestId) {
      isLoading.value = false
    }
  }
}

onMounted(async () => {
  startRelativeTimeRefresh()

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
  isMineScope
], () => {
  if (!authState.initialized) {
    return
  }

  loadSubmissions()
})

watch(authenticatedBearerToken, (nextToken, previousToken) => {
  if (!authState.initialized || nextToken === previousToken) {
    return
  }

  loadSubmissions()
})

onUnmounted(() => {
  stopRelativeTimeRefresh()
  resetCopyState()
})
</script>

<style scoped>
.submissions-panel {
  display: grid;
  gap: 1.25rem;
}

.submissions-header {
  display: flex;
  justify-content: flex-start;
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

.submissions-toolbar,
.submission-summary-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 1rem;
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
}

@media (max-width: 720px) {
  .submission-table {
    min-width: 980px;
  }
}
</style>
