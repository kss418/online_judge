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
            <span>닉네임</span>
            <span>문제번호</span>
            <span>결과</span>
            <span>시간</span>
            <span>메모리</span>
            <span>언어</span>
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
          </div>
        </div>
      </div>
    </article>
  </section>

  <Teleport to="body">
    <div
      v-if="sourceDialogOpen"
      class="submission-source-backdrop"
      @click.self="closeSourceDialog"
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
import { computed, ref, watch } from 'vue'
import { useRoute } from 'vue-router'

import { getSubmissionList, getSubmissionSource } from '@/api/submission'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const route = useRoute()
const { authState, isAuthenticated, initializeAuth } = useAuth()
const listLimit = 50
const isLoading = ref(true)
const errorMessage = ref('')
const submissions = ref([])
const sourceDialogOpen = ref(false)
const isLoadingSource = ref(false)
const sourceErrorMessage = ref('')
const sourceDetail = ref(null)
const activeSourceSubmissionId = ref(null)
const copyState = ref('idle')
const countFormatter = new Intl.NumberFormat()
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

function formatCount(value){
  return countFormatter.format(value)
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

  if (authState.currentUser.is_admin) {
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
  sourceDialogOpen.value = false
  isLoadingSource.value = false
  sourceErrorMessage.value = ''
  sourceDetail.value = null
  activeSourceSubmissionId.value = null
  resetCopyState()
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

async function loadSubmissions(){
  isLoading.value = true
  errorMessage.value = ''

  if (isMineScope.value && authState.isInitializing) {
    submissions.value = []
    return
  }

  if (isMineScope.value && !isAuthenticated.value) {
    submissions.value = []
    errorMessage.value = '내 제출을 보려면 로그인하세요.'
    isLoading.value = false
    return
  }

  try {
    const response = await getSubmissionList({
      limit: listLimit,
      problemId: numericProblemId.value ?? undefined,
      userId: activeUserId.value ?? undefined
    })

    submissions.value = Array.isArray(response.submissions)
      ? response.submissions
        .map((submission) => ({
          ...submission,
          submission_id: Number(submission.submission_id),
          user_id: Number(submission.user_id),
          problem_id: Number(submission.problem_id),
          user_name: submission.user_name || `사용자 ${countFormatter.format(submission.user_id)}`,
          elapsed_ms: typeof submission.elapsed_ms === 'number' ? submission.elapsed_ms : null,
          max_rss_kb: typeof submission.max_rss_kb === 'number' ? submission.max_rss_kb : null
        }))
        .sort((left, right) => right.submission_id - left.submission_id)
      : []
  } catch (error) {
    errorMessage.value = error instanceof Error
      ? error.message
      : '제출 목록을 불러오지 못했습니다.'
    submissions.value = []
  } finally {
    isLoading.value = false
  }
}

initializeAuth()

watch([
  () => route.name,
  numericProblemId,
  numericUserId,
  isMineScope,
  () => authState.isInitializing,
  () => authState.currentUser?.id
], () => {
  loadSubmissions()
}, { immediate: true })
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
  min-width: 900px;
  border: 1px solid var(--line);
  border-radius: 1.25rem;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.76);
}

.submission-table-head,
.submission-row {
  display: grid;
  grid-template-columns:
    minmax(88px, 0.9fr)
    minmax(140px, 1.35fr)
    minmax(96px, 0.95fr)
    minmax(120px, 1.2fr)
    minmax(96px, 0.95fr)
    minmax(112px, 1fr)
    minmax(96px, 0.9fr);
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

.submission-cell.is-number,
.submission-cell.is-user,
.submission-cell.is-metric,
.submission-cell.is-language {
  font-weight: 600;
}

.submission-problem-link {
  min-width: 0;
  font-weight: 700;
  color: var(--ink-strong);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  transition: color 160ms ease;
}

.submission-problem-link:hover {
  color: var(--accent);
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
}

.submission-language-button:hover {
  text-decoration: underline;
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
    min-width: 860px;
  }
}
</style>
