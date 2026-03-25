<template>
  <section class="page-grid single-column">
    <article class="panel submissions-panel">
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
            <span class="submission-cell is-language">
              {{ submission.language }}
            </span>
          </div>
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, ref, watch } from 'vue'
import { useRoute } from 'vue-router'

import { getSubmissionList } from '@/api/submission'
import StatusBadge from '@/components/StatusBadge.vue'

const route = useRoute()
const listLimit = 50
const isLoading = ref(true)
const errorMessage = ref('')
const submissions = ref([])
const countFormatter = new Intl.NumberFormat()
const submissionCount = computed(() => submissions.value.length)
const numericProblemId = computed(() => {
  const problemIdQuery = Array.isArray(route.query.problemId)
    ? route.query.problemId[0]
    : route.query.problemId

  const parsedProblemId = Number.parseInt(problemIdQuery, 10)
  return Number.isInteger(parsedProblemId) && parsedProblemId > 0
    ? parsedProblemId
    : null
})
const pageTitle = computed(() =>
  numericProblemId.value
    ? `문제 #${formatCount(numericProblemId.value)} 제출 목록`
    : '제출 목록'
)

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

async function loadSubmissions(){
  isLoading.value = true
  errorMessage.value = ''

  try {
    const response = await getSubmissionList({
      limit: listLimit,
      problemId: numericProblemId.value ?? undefined
    })

    submissions.value = Array.isArray(response.submissions)
      ? response.submissions
        .map((submission) => ({
          ...submission,
          submission_id: Number(submission.submission_id),
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

watch(numericProblemId, () => {
  loadSubmissions()
}, { immediate: true })
</script>

<style scoped>
.submissions-panel {
  display: grid;
  gap: 1.25rem;
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

@media (max-width: 720px) {
  .submission-table {
    min-width: 860px;
  }
}
</style>
