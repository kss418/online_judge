<template>
  <section class="page-grid single-column">
    <article class="panel detail-panel">
      <div class="detail-header">
        <RouterLink class="detail-back-link" :to="{ name: 'problems' }">
          문제 목록으로
        </RouterLink>
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>문제 정보를 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <template v-else-if="problemDetail">
        <div class="panel-header detail-title-block">
          <div>
            <p class="panel-kicker">problem</p>
            <div class="detail-title-row">
              <div class="detail-title-copy">
                <h3>#{{ problemDetail.problem_id }} {{ problemDetail.title }}</h3>
                <StatusBadge
                  v-if="problemDetail.user_problem_state"
                  :label="getProblemStateLabel(problemDetail.user_problem_state)"
                  :tone="getProblemStateTone(problemDetail.user_problem_state)"
                />
              </div>
              <div class="detail-title-actions">
                <RouterLink
                  v-if="isAuthenticated"
                  class="ghost-button detail-list-link"
                  :to="{ name: 'problem-my-submissions', params: { problemId: problemDetail.problem_id } }"
                >
                  내 제출
                </RouterLink>
                <RouterLink
                  class="ghost-button detail-list-link"
                  :to="{ name: 'problem-submissions', params: { problemId: problemDetail.problem_id } }"
                >
                  제출 목록
                </RouterLink>
                <RouterLink
                  class="primary-button detail-submit-link"
                  :to="{ name: 'problem-submit', params: { problemId: problemDetail.problem_id } }"
                >
                  제출하기
                </RouterLink>
              </div>
            </div>
          </div>
        </div>

        <div class="detail-metrics">
          <div class="detail-metric-card">
            <span class="detail-metric-label">메모리 제한</span>
            <strong>{{ formatCount(problemDetail.limits.memory_limit_mb) }} MB</strong>
          </div>
          <div class="detail-metric-card">
            <span class="detail-metric-label">시간 제한</span>
            <strong>{{ formatCount(problemDetail.limits.time_limit_ms) }} ms</strong>
          </div>
          <div class="detail-metric-card">
            <span class="detail-metric-label">정답</span>
            <strong>{{ formatCount(problemDetail.statistics.accepted_count) }}</strong>
          </div>
          <div class="detail-metric-card">
            <span class="detail-metric-label">제출</span>
            <strong>{{ formatCount(problemDetail.statistics.submission_count) }}</strong>
          </div>
          <div class="detail-metric-card">
            <span class="detail-metric-label">정답률</span>
            <strong>{{ acceptanceRate }}</strong>
          </div>
        </div>

        <div class="detail-sections">
          <article class="detail-section">
            <div class="panel-header">
              <div>
                <p class="panel-kicker">description</p>
                <h3>문제 설명</h3>
              </div>
            </div>
            <p class="detail-text">{{ problemDetail.statement.description }}</p>
          </article>

          <article class="detail-section">
            <div class="panel-header">
              <div>
                <p class="panel-kicker">input</p>
                <h3>입력</h3>
              </div>
            </div>
            <p class="detail-text">{{ problemDetail.statement.input_format }}</p>
          </article>

          <article class="detail-section">
            <div class="panel-header">
              <div>
                <p class="panel-kicker">output</p>
                <h3>출력</h3>
              </div>
            </div>
            <p class="detail-text">{{ problemDetail.statement.output_format }}</p>
          </article>

          <article
            v-if="problemDetail.statement.note"
            class="detail-section"
          >
            <div class="panel-header">
              <div>
                <p class="panel-kicker">note</p>
                <h3>비고</h3>
              </div>
            </div>
            <p class="detail-text">{{ problemDetail.statement.note }}</p>
          </article>
        </div>

        <article class="detail-section">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">samples</p>
              <h3>예제</h3>
            </div>
          </div>

          <div v-if="!problemDetail.samples.length" class="empty-state">
            <p>등록된 예제가 아직 없습니다.</p>
          </div>

          <div v-else class="sample-list">
            <div
              v-for="sample in problemDetail.samples"
              :key="sample.sample_order"
              class="sample-card"
            >
              <strong class="sample-title">예제 {{ sample.sample_order }}</strong>
              <div class="sample-grid">
                <div class="sample-block">
                  <span class="sample-label">입력</span>
                  <pre>{{ sample.sample_input }}</pre>
                </div>
                <div class="sample-block">
                  <span class="sample-label">출력</span>
                  <pre>{{ sample.sample_output }}</pre>
                </div>
              </div>
            </div>
          </div>
        </article>
      </template>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute } from 'vue-router'

import { getProblemDetail } from '@/api/problem'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'
import {
  getProblemStateLabel,
  getProblemStateTone
} from '@/utils/problemState'

const route = useRoute()
const { authState, isAuthenticated, initializeAuth } = useAuth()
const isLoading = ref(true)
const errorMessage = ref('')
const problemDetail = ref(null)
const hasLoadedOnce = ref(false)
const countFormatter = new Intl.NumberFormat()
const rateFormatter = new Intl.NumberFormat('ko-KR', {
  minimumFractionDigits: 1,
  maximumFractionDigits: 1
})

const numericProblemId = computed(() => Number.parseInt(route.params.problemId, 10))
const authenticatedBearerToken = computed(() =>
  authState.initialized && isAuthenticated.value ? authState.token : ''
)
let latestLoadRequestId = 0

const acceptanceRate = computed(() => {
  const statistics = problemDetail.value?.statistics

  if (!statistics || statistics.submission_count <= 0) {
    return '-'
  }

  const rate = (statistics.accepted_count / statistics.submission_count) * 100
  return `${rateFormatter.format(rate)}%`
})

watch(numericProblemId, () => {
  loadProblemDetail()
})

async function loadProblemDetail(){
  const requestId = ++latestLoadRequestId
  isLoading.value = true
  errorMessage.value = ''

  if (!Number.isInteger(numericProblemId.value) || numericProblemId.value <= 0) {
    errorMessage.value = '올바르지 않은 문제 번호입니다.'
    problemDetail.value = null
    isLoading.value = false
    return
  }

  try {
    const response = await getProblemDetail(numericProblemId.value, {
      bearerToken: authenticatedBearerToken.value
    })

    if (requestId !== latestLoadRequestId) {
      return
    }

    problemDetail.value = {
      ...response,
      limits: response.limits || {
        memory_limit_mb: 0,
        time_limit_ms: 0
      },
      statistics: {
        accepted_count: Number(response.statistics?.accepted_count ?? 0),
        submission_count: Number(response.statistics?.submission_count ?? 0)
      },
      statement: {
        description: response.statement?.description ?? '',
        input_format: response.statement?.input_format ?? '',
        output_format: response.statement?.output_format ?? '',
        note: response.statement?.note ?? ''
      },
      samples: Array.isArray(response.samples) ? response.samples : []
    }
    hasLoadedOnce.value = true
  } catch (error) {
    if (requestId !== latestLoadRequestId) {
      return
    }

    errorMessage.value = error instanceof Error
      ? error.message
      : '문제 정보를 불러오지 못했습니다.'
    problemDetail.value = null
    hasLoadedOnce.value = true
  } finally {
    if (requestId === latestLoadRequestId) {
      isLoading.value = false
    }
  }
}

function formatCount(value){
  return countFormatter.format(value)
}

onMounted(async () => {
  if (!authState.initialized) {
    await initializeAuth()
  }

  if (!hasLoadedOnce.value) {
    loadProblemDetail()
  }
})

watch(authenticatedBearerToken, (nextToken, previousToken) => {
  if (nextToken === previousToken) {
    return
  }

  loadProblemDetail()
})
</script>

<style scoped>
.detail-panel {
  display: grid;
  gap: 1.25rem;
}

.detail-header {
  display: flex;
  justify-content: flex-start;
}

.detail-back-link {
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

.detail-back-link:hover {
  transform: translateY(-1px);
  border-color: rgba(20, 33, 61, 0.24);
}

.detail-title-block {
  align-items: start;
}

.detail-title-row {
  display: flex;
  gap: 0.9rem;
  align-items: center;
  flex-wrap: wrap;
}

.detail-title-copy {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.detail-title-actions {
  display: inline-flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.detail-rate-label,
.detail-metric-label,
.sample-label {
  color: var(--ink-soft);
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.08em;
}

.detail-metrics {
  display: grid;
  grid-template-columns: repeat(5, minmax(0, 1fr));
  gap: 0.9rem;
}

.detail-metric-card,
.detail-section,
.sample-card {
  border: 1px solid var(--line);
  border-radius: 20px;
  background: var(--surface-strong);
}

.detail-metric-card {
  display: grid;
  gap: 0.35rem;
  padding: 1rem 1.1rem;
}

.detail-sections {
  display: grid;
  gap: 1rem;
}

.detail-submit-link {
  display: inline-flex;
  align-items: center;
  justify-content: center;
}

.detail-list-link {
  display: inline-flex;
  align-items: center;
  justify-content: center;
}

.detail-section {
  padding: 1.2rem;
}

.detail-text {
  margin: 0;
  min-height: 1.5rem;
  white-space: pre-wrap;
  color: var(--ink-strong);
}

.sample-list {
  display: grid;
  gap: 1rem;
}

.sample-card {
  padding: 1rem 1.1rem;
}

.sample-title {
  display: block;
  margin-bottom: 0.85rem;
}

.sample-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 0.9rem;
}

.sample-block {
  display: grid;
  gap: 0.45rem;
}

.sample-block pre {
  margin: 0;
  min-height: 5rem;
  padding: 0.9rem 1rem;
  border-radius: 16px;
  background: rgba(20, 33, 61, 0.06);
  overflow: auto;
  font: inherit;
  font-family: "SFMono-Regular", "Consolas", monospace;
}

@media (max-width: 900px) {
  .detail-metrics,
  .sample-grid {
    grid-template-columns: minmax(0, 1fr);
  }

  .detail-title-block {
    align-items: start;
  }

  .detail-title-row {
    align-items: stretch;
  }

  .detail-rate-block {
    justify-items: start;
  }
}
</style>
