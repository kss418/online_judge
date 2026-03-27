<template>
  <section class="page-grid single-column">
    <article class="panel admin-testcases-shell">
      <div class="admin-testcases-toolbar">
        <div>
          <p class="panel-kicker">Testcase Admin</p>
          <h3>테스트케이스 관리</h3>
          <p class="admin-testcases-copy">
            왼쪽에서 문제를 고른 뒤 테스트케이스를 확인하고, 마지막 항목 추가와 삭제, 선택 항목 수정까지 한 화면에서 처리할 수 있습니다.
          </p>
        </div>

        <div class="admin-testcases-toolbar-actions">
          <StatusBadge
            :label="toolbarStatusLabel"
            :tone="toolbarStatusTone"
          />
          <button
            v-if="canManageProblems"
            type="button"
            class="ghost-button"
            :disabled="Boolean(busySection) || isLoadingProblems || isLoadingProblem || isLoadingTestcases"
            @click="refreshPage"
          >
            새로고침
          </button>
          <RouterLink
            v-if="selectedProblemId > 0"
            class="ghost-button"
            :to="{ name: 'problem-detail', params: { problemId: selectedProblemId } }"
          >
            문제 보기
          </RouterLink>
        </div>
      </div>

      <div v-if="authState.isInitializing" class="empty-state">
        <p>관리자 권한을 확인하는 중입니다.</p>
      </div>

      <div v-else-if="!isAuthenticated" class="empty-state">
        <p>테스트케이스 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.</p>
      </div>

      <div v-else-if="!canManageProblems" class="empty-state error-state">
        <p>이 페이지는 관리자만 접근할 수 있습니다.</p>
      </div>

      <template v-else>
        <div class="admin-testcases-layout">
          <aside class="admin-testcases-problem-list-panel">
            <form class="admin-testcases-problem-search" @submit.prevent="submitSearch">
              <label class="sr-only" for="admin-testcase-problem-search">문제 검색</label>
              <input
                id="admin-testcase-problem-search"
                v-model="searchInput"
                class="field-input admin-testcases-problem-search-input"
                type="search"
                placeholder="문제 제목 검색"
              />
              <button
                type="submit"
                class="ghost-button"
                :disabled="isLoadingProblems || Boolean(busySection)"
              >
                검색
              </button>
              <button
                v-if="searchKeyword"
                type="button"
                class="ghost-button"
                :disabled="isLoadingProblems || Boolean(busySection)"
                @click="resetSearch"
              >
                초기화
              </button>
            </form>

            <div class="admin-testcases-problem-list-header">
              <p class="admin-testcases-problem-list-caption">
                {{ searchKeyword ? `"${searchKeyword}" 검색 결과` : '전체 문제' }}
              </p>
              <span class="admin-testcases-problem-list-count">{{ formatCount(problemCount) }}개</span>
            </div>

            <div v-if="isLoadingProblems" class="empty-state compact-state">
              <p>문제 목록을 불러오는 중입니다.</p>
            </div>

            <div v-else-if="listErrorMessage" class="empty-state error-state compact-state">
              <p>{{ listErrorMessage }}</p>
            </div>

            <div v-else-if="!problemCount" class="empty-state compact-state">
              <p>
                {{ searchKeyword ? '검색 조건에 맞는 문제가 없습니다.' : '등록된 문제가 아직 없습니다.' }}
              </p>
            </div>

            <div v-else class="admin-testcases-problem-list">
              <button
                v-for="problem in problems"
                :key="problem.problem_id"
                type="button"
                class="admin-testcases-problem-item"
                :class="{ 'is-active': problem.problem_id === selectedProblemId }"
                @click="selectProblem(problem.problem_id)"
              >
                <div class="admin-testcases-problem-item-head">
                  <strong>#{{ formatCount(problem.problem_id) }}</strong>
                  <span class="admin-testcases-problem-item-version">v{{ problem.version }}</span>
                </div>
                <strong class="admin-testcases-problem-item-title">{{ problem.title }}</strong>
                <div class="admin-testcases-problem-item-limits">
                  <span>{{ formatProblemLimit(problem.time_limit_ms, 'ms') }}</span>
                  <span>{{ formatProblemLimit(problem.memory_limit_mb, 'MB') }}</span>
                </div>
              </button>
            </div>
          </aside>

          <section class="admin-testcases-editor-panel">
            <div v-if="!selectedProblemId" class="empty-state">
              <p>왼쪽 목록에서 문제를 선택하면 테스트케이스를 관리할 수 있습니다.</p>
            </div>

            <div v-else-if="isLoadingProblem" class="empty-state">
              <p>문제 정보를 불러오는 중입니다.</p>
            </div>

            <div v-else-if="problemErrorMessage" class="empty-state error-state">
              <p>{{ problemErrorMessage }}</p>
            </div>

            <template v-else-if="problemDetail">
              <div class="admin-testcases-editor-header">
                <div>
                  <p class="panel-kicker">editor</p>
                  <h3>#{{ formatCount(problemDetail.problem_id) }} {{ problemDetail.title }}</h3>
                  <p class="admin-testcases-editor-copy">
                    요약 리스트에서 특정 테스트케이스를 고르고, 선택한 항목만 편집해서 저장할 수 있습니다.
                  </p>
                </div>
                <div class="admin-testcases-editor-side">
                  <p class="admin-testcases-editor-current-problem">
                    #{{ formatCount(problemDetail.problem_id) }} {{ problemDetail.title }}
                  </p>
                  <div class="admin-testcases-editor-actions">
                    <RouterLink
                      class="ghost-button"
                      :to="{
                        name: 'admin-problems',
                        query: { problemId: String(problemDetail.problem_id) }
                      }"
                    >
                      문제 관리
                    </RouterLink>
                    <RouterLink
                      class="ghost-button"
                      :to="{ name: 'problem-detail', params: { problemId: problemDetail.problem_id } }"
                    >
                      문제 보기
                    </RouterLink>
                  </div>
                </div>
              </div>

              <article class="admin-testcases-section">
                <div class="panel-header">
                  <div>
                    <p class="panel-kicker">append</p>
                    <h3>테스트케이스 추가</h3>
                  </div>
                </div>

                <div class="admin-testcases-grid">
                  <label class="field-block">
                    <span class="field-label">입력</span>
                    <textarea
                      v-model="newTestcaseInput"
                      class="admin-testcases-textarea"
                      spellcheck="false"
                      :disabled="Boolean(busySection)"
                      placeholder="빈 입력도 허용됩니다."
                    />
                  </label>

                  <label class="field-block">
                    <span class="field-label">출력</span>
                    <textarea
                      v-model="newTestcaseOutput"
                      class="admin-testcases-textarea"
                      spellcheck="false"
                      :disabled="Boolean(busySection)"
                      placeholder="빈 출력도 허용됩니다."
                    />
                  </label>
                </div>

                <div class="admin-testcases-actions">
                  <button
                    type="button"
                    class="primary-button"
                    :disabled="!canCreateTestcase"
                    @click="handleCreateTestcase"
                  >
                    {{ isCreatingTestcase ? '추가 중...' : '테스트케이스 추가' }}
                  </button>
                </div>
              </article>

              <article class="admin-testcases-section">
                <div class="panel-header">
                  <div>
                    <p class="panel-kicker">list</p>
                    <h3>테스트케이스 요약</h3>
                  </div>
                </div>

                <form class="admin-testcases-order-form" @submit.prevent="handleViewSelectedTestcase">
                  <label class="field-block admin-testcases-order-field">
                    <span class="field-label">특정 순번 보기</span>
                    <input
                      v-model="viewTestcaseOrderInput"
                      class="field-input"
                      type="text"
                      inputmode="numeric"
                      :disabled="isLoadingTestcases || !testcaseItems.length"
                      placeholder="예: 37"
                    />
                  </label>

                  <div class="admin-testcases-order-actions">
                    <button
                      type="submit"
                      class="ghost-button"
                      :disabled="!canViewSpecificTestcase"
                    >
                      보기
                    </button>
                    <button
                      v-if="selectedTestcase"
                      type="button"
                      class="ghost-button"
                      :disabled="isLoadingTestcases || !testcaseItems.length"
                      @click="selectTestcase(testcaseItems[0].testcase_order)"
                    >
                      처음으로
                    </button>
                  </div>
                </form>

                <div v-if="isLoadingTestcases" class="empty-state compact-state">
                  <p>테스트케이스를 불러오는 중입니다.</p>
                </div>

                <div v-else-if="testcaseErrorMessage" class="empty-state error-state compact-state">
                  <p>{{ testcaseErrorMessage }}</p>
                </div>

                <div v-else-if="!testcaseItems.length" class="empty-state compact-state">
                  <p>등록된 테스트케이스가 아직 없습니다.</p>
                </div>

                <div v-else class="admin-testcases-summary-layout">
                  <div class="admin-testcases-summary-panel">
                    <div class="admin-testcases-summary-list">
                      <button
                        v-for="testcase in testcaseItems"
                        :key="testcase.testcase_order"
                        type="button"
                        class="admin-testcase-summary-item"
                        :class="{ 'is-active': testcase.testcase_order === selectedTestcaseOrder }"
                        :ref="(element) => setTestcaseSummaryElement(testcase.testcase_order, element)"
                        @click="selectTestcase(testcase.testcase_order)"
                      >
                        <div class="admin-testcase-summary-head">
                          <strong>#{{ formatCount(testcase.testcase_order) }}</strong>
                          <span
                            v-if="isLastTestcase(testcase.testcase_order)"
                            class="admin-testcase-summary-badge"
                          >
                            last
                          </span>
                        </div>
                        <span class="admin-testcase-summary-copy">
                          입력 {{ describeTestcaseContent(testcase.testcase_input) }}
                        </span>
                        <span class="admin-testcase-summary-copy">
                          출력 {{ describeTestcaseContent(testcase.testcase_output) }}
                        </span>
                      </button>
                    </div>
                  </div>

                  <article
                    v-if="selectedTestcase"
                    class="admin-testcase-card"
                  >
                    <div class="admin-testcase-card-header">
                      <div>
                        <p class="panel-kicker">testcase {{ formatCount(selectedTestcase.testcase_order) }}</p>
                        <h3>테스트케이스 {{ formatCount(selectedTestcase.testcase_order) }}</h3>
                      </div>

                      <button
                        v-if="isLastTestcase(selectedTestcase.testcase_order)"
                        type="button"
                        class="ghost-button admin-testcase-delete-button"
                        :disabled="!canDeleteLastTestcase"
                        @click="handleDeleteLastTestcase"
                      >
                        {{ isDeletingLastTestcase ? '삭제 중...' : '테스트케이스 삭제' }}
                      </button>
                    </div>

                    <div class="admin-testcases-grid">
                      <label class="field-block">
                        <span class="field-label">입력</span>
                        <textarea
                          v-model="selectedTestcaseInputDraft"
                          class="admin-testcases-textarea"
                          spellcheck="false"
                          :disabled="Boolean(busySection)"
                          placeholder="빈 입력도 허용됩니다."
                        />
                      </label>

                      <label class="field-block">
                        <span class="field-label">출력</span>
                        <textarea
                          v-model="selectedTestcaseOutputDraft"
                          class="admin-testcases-textarea"
                          spellcheck="false"
                          :disabled="Boolean(busySection)"
                          placeholder="빈 출력도 허용됩니다."
                        />
                      </label>
                    </div>

                    <div class="admin-testcases-actions">
                      <button
                        type="button"
                        class="primary-button"
                        :disabled="!canSaveSelectedTestcase"
                        @click="handleSaveSelectedTestcase"
                      >
                        {{ isSavingSelectedTestcase ? '저장 중...' : '저장' }}
                      </button>
                    </div>
                  </article>
                </div>
              </article>
            </template>
          </section>
        </div>
      </template>
    </article>
  </section>
</template>

<script setup>
import { computed, nextTick, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import {
  createProblemTestcase,
  deleteProblemTestcase,
  getProblemDetail,
  getProblemList,
  getProblemTestcases,
  updateProblemTestcase
} from '@/api/problem'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'
import { useNotice } from '@/composables/useNotice'

const route = useRoute()
const router = useRouter()
const { authState, isAuthenticated, initializeAuth } = useAuth()
const { showErrorNotice, showSuccessNotice } = useNotice()
const countFormatter = new Intl.NumberFormat()

const selectedProblemId = computed(() => {
  const parsedValue = Number.parseInt(route.params.problemId, 10)
  return Number.isInteger(parsedValue) && parsedValue > 0 ? parsedValue : 0
})

const isLoadingProblems = ref(true)
const isLoadingProblem = ref(false)
const isLoadingTestcases = ref(false)
const listErrorMessage = ref('')
const problemErrorMessage = ref('')
const testcaseErrorMessage = ref('')
const searchInput = ref('')
const searchKeyword = ref('')
const problems = ref([])
const problemDetail = ref(null)
const testcaseItems = ref([])
const newTestcaseInput = ref('')
const newTestcaseOutput = ref('')
const selectedTestcaseOrder = ref(0)
const selectedTestcaseInputDraft = ref('')
const selectedTestcaseOutputDraft = ref('')
const viewTestcaseOrderInput = ref('')
const busySection = ref('')
const testcaseSummaryElementMap = new Map()

let latestProblemListRequestId = 0
let latestProblemRequestId = 0
let latestTestcaseRequestId = 0
let latestProblemLimitHydrationId = 0

const canManageProblems = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 1)
const problemCount = computed(() => problems.value.length)
const testcaseCount = computed(() => testcaseItems.value.length)
const toolbarStatusLabel = computed(() => {
  if (isLoadingProblems.value || isLoadingProblem.value || isLoadingTestcases.value) {
    return 'Loading'
  }

  if (selectedProblemId.value > 0) {
    return `${formatCount(testcaseCount.value)} Testcases`
  }

  return `${formatCount(problemCount.value)} Problems`
})
const toolbarStatusTone = computed(() => {
  if (listErrorMessage.value || problemErrorMessage.value || testcaseErrorMessage.value) {
    return 'danger'
  }

  return 'success'
})
const isCreatingTestcase = computed(() => busySection.value === 'create')
const isDeletingLastTestcase = computed(() => busySection.value === 'delete-last')
const isSavingSelectedTestcase = computed(() => busySection.value === 'save')
const selectedTestcase = computed(() =>
  testcaseItems.value.find((testcase) => testcase.testcase_order === selectedTestcaseOrder.value) || null
)
const canCreateTestcase = computed(() =>
  selectedProblemId.value > 0 &&
  Boolean(authState.token) &&
  !busySection.value
)
const canDeleteLastTestcase = computed(() =>
  selectedProblemId.value > 0 &&
  Boolean(authState.token) &&
  Boolean(testcaseItems.value.length) &&
  !busySection.value
)
const canSaveSelectedTestcase = computed(() => {
  if (!selectedTestcase.value || !authState.token || busySection.value) {
    return false
  }

  return (
    selectedTestcaseInputDraft.value !== selectedTestcase.value.testcase_input ||
    selectedTestcaseOutputDraft.value !== selectedTestcase.value.testcase_output
  )
})
const canViewSpecificTestcase = computed(() => {
  if (isLoadingTestcases.value || !testcaseItems.value.length) {
    return false
  }

  return parsePositiveInteger(viewTestcaseOrderInput.value) != null
})

watch(selectedProblemId, () => {
  resetSelectedProblemState()

  if (!authState.initialized || !canManageProblems.value) {
    return
  }

  if (selectedProblemId.value > 0) {
    void loadSelectedProblemData()
    return
  }

  isLoadingProblem.value = false
  isLoadingTestcases.value = false
})

watch(selectedTestcase, (testcase) => {
  selectedTestcaseInputDraft.value = testcase?.testcase_input ?? ''
  selectedTestcaseOutputDraft.value = testcase?.testcase_output ?? ''
}, {
  immediate: true
})

function formatCount(value){
  return countFormatter.format(Number(value) || 0)
}

function formatProblemLimit(value, unit){
  const numericValue = Number(value ?? 0)
  if (!numericValue) {
    return `미설정 ${unit}`
  }

  return `${formatCount(numericValue)} ${unit}`
}

function normalizeProblemItem(problem){
  return {
    ...problem,
    problem_id: Number(problem.problem_id ?? 0),
    version: Number(problem.version ?? 1),
    title: problem.title || '',
    time_limit_ms: Number(problem.time_limit_ms ?? problem.limits?.time_limit_ms ?? 0),
    memory_limit_mb: Number(problem.memory_limit_mb ?? problem.limits?.memory_limit_mb ?? 0)
  }
}

function normalizeProblemDetail(detail){
  return {
    ...detail,
    problem_id: Number(detail.problem_id ?? 0),
    title: detail.title || ''
  }
}

function normalizeTestcaseList(response){
  const rawTestcases = Array.isArray(response?.testcases) ? response.testcases : []

  return rawTestcases.map((testcase) => ({
    testcase_id: Number(testcase.testcase_id ?? 0),
    testcase_order: Number(testcase.testcase_order ?? 0),
    testcase_input: typeof testcase.testcase_input === 'string' ? testcase.testcase_input : '',
    testcase_output: typeof testcase.testcase_output === 'string' ? testcase.testcase_output : ''
  }))
}

function parsePositiveInteger(value){
  const normalized = typeof value === 'string' ? value.trim() : String(value ?? '').trim()
  if (!normalized) {
    return null
  }

  const parsedValue = Number.parseInt(normalized, 10)
  if (!Number.isInteger(parsedValue) || parsedValue <= 0) {
    return null
  }

  return parsedValue
}

function describeTestcaseContent(value){
  if (!value) {
    return '빈 값'
  }

  return `${formatCount(value.length)}자 · ${formatCount(value.split('\n').length)}줄`
}

function isLastTestcase(testcaseOrder){
  if (!testcaseItems.value.length) {
    return false
  }

  return testcaseItems.value[testcaseItems.value.length - 1].testcase_order === testcaseOrder
}

function resetSelectedProblemState(){
  problemDetail.value = null
  testcaseItems.value = []
  newTestcaseInput.value = ''
  newTestcaseOutput.value = ''
  selectedTestcaseOrder.value = 0
  selectedTestcaseInputDraft.value = ''
  selectedTestcaseOutputDraft.value = ''
  viewTestcaseOrderInput.value = ''
  problemErrorMessage.value = ''
  testcaseErrorMessage.value = ''
}

function syncSelectedTestcase(preferredOrder){
  if (!testcaseItems.value.length) {
    selectedTestcaseOrder.value = 0
    selectedTestcaseInputDraft.value = ''
    selectedTestcaseOutputDraft.value = ''
    viewTestcaseOrderInput.value = ''
    return
  }

  const lastTestcaseOrder = testcaseItems.value[testcaseItems.value.length - 1].testcase_order
  const candidateOrder =
    preferredOrder && preferredOrder > 0
      ? Math.min(preferredOrder, lastTestcaseOrder)
      : selectedTestcaseOrder.value > 0
        ? Math.min(selectedTestcaseOrder.value, lastTestcaseOrder)
        : testcaseItems.value[0].testcase_order
  const matchedTestcase = testcaseItems.value.find(
    (testcase) => testcase.testcase_order === candidateOrder
  )
  const nextOrder = matchedTestcase
    ? matchedTestcase.testcase_order
    : testcaseItems.value[0].testcase_order

  selectedTestcaseOrder.value = nextOrder
  viewTestcaseOrderInput.value = String(nextOrder)
  void scrollSelectedTestcaseIntoView()
}

function selectTestcase(testcaseOrder){
  selectedTestcaseOrder.value = testcaseOrder
  viewTestcaseOrderInput.value = String(testcaseOrder)
  void scrollSelectedTestcaseIntoView()
}

function setTestcaseSummaryElement(testcaseOrder, element){
  if (!element) {
    testcaseSummaryElementMap.delete(testcaseOrder)
    return
  }

  testcaseSummaryElementMap.set(testcaseOrder, element)
}

async function scrollSelectedTestcaseIntoView(){
  await nextTick()

  const summaryElement = testcaseSummaryElementMap.get(selectedTestcaseOrder.value)
  if (!summaryElement || typeof summaryElement.scrollIntoView !== 'function') {
    return
  }

  summaryElement.scrollIntoView({
    block: 'nearest',
    inline: 'nearest'
  })
}

async function selectProblem(problemId){
  if (problemId === selectedProblemId.value) {
    return
  }

  await router.push({
    name: 'admin-problem-testcases',
    params: {
      problemId: String(problemId)
    }
  })
}

function submitSearch(){
  searchKeyword.value = searchInput.value.trim()
  void loadProblems()
}

function resetSearch(){
  searchInput.value = ''
  searchKeyword.value = ''
  void loadProblems()
}

function handleViewSelectedTestcase(){
  const targetOrder = parsePositiveInteger(viewTestcaseOrderInput.value)
  if (targetOrder == null) {
    showErrorNotice('확인할 테스트케이스 순번을 올바르게 입력하세요.')
    return
  }

  const matchedTestcase = testcaseItems.value.find(
    (testcase) => testcase.testcase_order === targetOrder
  )
  if (!matchedTestcase) {
    showErrorNotice(`테스트케이스 ${targetOrder}번이 없습니다.`)
    return
  }

  selectTestcase(matchedTestcase.testcase_order)
}

async function loadProblems(){
  const requestId = ++latestProblemListRequestId
  isLoadingProblems.value = true
  listErrorMessage.value = ''

  try {
    const response = await getProblemList({
      title: searchKeyword.value,
      bearerToken: authState.token || ''
    })

    if (requestId !== latestProblemListRequestId) {
      return
    }

    problems.value = Array.isArray(response.problems)
      ? response.problems.map(normalizeProblemItem)
      : []

    void hydrateProblemLimits(problems.value.map((problem) => problem.problem_id))
  } catch (error) {
    if (requestId !== latestProblemListRequestId) {
      return
    }

    problems.value = []
    listErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 목록을 불러오지 못했습니다.'
  } finally {
    if (requestId === latestProblemListRequestId) {
      isLoadingProblems.value = false
    }
  }
}

async function hydrateProblemLimits(problemIds){
  if (!problemIds.length) {
    return
  }

  const hydrationId = ++latestProblemLimitHydrationId
  const detailEntries = await Promise.all(problemIds.map(async (problemId) => {
    try {
      const response = await getProblemDetail(problemId, {
        bearerToken: authState.token || ''
      })
      const detail = normalizeProblemDetail(response)
      return [
        problemId,
        {
          time_limit_ms: detail.limits?.time_limit_ms ?? 0,
          memory_limit_mb: detail.limits?.memory_limit_mb ?? 0
        }
      ]
    } catch {
      return null
    }
  }))

  if (hydrationId !== latestProblemLimitHydrationId) {
    return
  }

  const limitMap = new Map(detailEntries.filter(Boolean))
  if (!limitMap.size) {
    return
  }

  problems.value = problems.value.map((problem) => {
    const limitPatch = limitMap.get(problem.problem_id)
    return limitPatch
      ? {
        ...problem,
        ...limitPatch
      }
      : problem
  })
}

async function loadProblemDetail(){
  const requestId = ++latestProblemRequestId
  isLoadingProblem.value = true
  problemErrorMessage.value = ''

  if (selectedProblemId.value <= 0) {
    problemDetail.value = null
    isLoadingProblem.value = false
    return
  }

  try {
    const response = await getProblemDetail(selectedProblemId.value, {
      bearerToken: authState.token || ''
    })

    if (requestId !== latestProblemRequestId) {
      return
    }

    problemDetail.value = normalizeProblemDetail(response)
  } catch (error) {
    if (requestId !== latestProblemRequestId) {
      return
    }

    problemDetail.value = null
    problemErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 정보를 불러오지 못했습니다.'
  } finally {
    if (requestId === latestProblemRequestId) {
      isLoadingProblem.value = false
    }
  }
}

async function loadTestcases(){
  const requestId = ++latestTestcaseRequestId
  isLoadingTestcases.value = true
  testcaseErrorMessage.value = ''

  if (!authState.token || selectedProblemId.value <= 0) {
    testcaseItems.value = []
    isLoadingTestcases.value = false
    return
  }

  try {
    const response = await getProblemTestcases(selectedProblemId.value, {
      bearerToken: authState.token
    })

    if (requestId !== latestTestcaseRequestId) {
      return
    }

    testcaseItems.value = normalizeTestcaseList(response)
    syncSelectedTestcase()
  } catch (error) {
    if (requestId !== latestTestcaseRequestId) {
      return
    }

    testcaseItems.value = []
    selectedTestcaseOrder.value = 0
    selectedTestcaseInputDraft.value = ''
    selectedTestcaseOutputDraft.value = ''
    viewTestcaseOrderInput.value = ''
    testcaseErrorMessage.value = error instanceof Error
      ? error.message
      : '테스트케이스를 불러오지 못했습니다.'
  } finally {
    if (requestId === latestTestcaseRequestId) {
      isLoadingTestcases.value = false
    }
  }
}

async function loadSelectedProblemData(){
  if (selectedProblemId.value <= 0) {
    isLoadingProblem.value = false
    isLoadingTestcases.value = false
    return
  }

  await Promise.all([
    loadProblemDetail(),
    loadTestcases()
  ])
}

async function refreshPage(){
  if (busySection.value) {
    return
  }

  await loadProblems()
  await loadSelectedProblemData()
}

async function handleCreateTestcase(){
  if (!canCreateTestcase.value || !authState.token) {
    return
  }

  busySection.value = 'create'

  const nextTestcaseInput = newTestcaseInput.value
  const nextTestcaseOutput = newTestcaseOutput.value

  try {
    const response = await createProblemTestcase(
      selectedProblemId.value,
      {
        testcase_input: nextTestcaseInput,
        testcase_output: nextTestcaseOutput
      },
      authState.token
    )

    testcaseItems.value = [
      ...testcaseItems.value,
      {
        testcase_id: Number(response.testcase_id ?? 0),
        testcase_order: Number(response.testcase_order ?? (testcaseItems.value.length + 1)),
        testcase_input: nextTestcaseInput,
        testcase_output: nextTestcaseOutput
      }
    ]
    syncSelectedTestcase(Number(response.testcase_order ?? 0))
    newTestcaseInput.value = ''
    newTestcaseOutput.value = ''
    showSuccessNotice('테스트케이스를 마지막에 추가했습니다.')
  } catch (error) {
    showErrorNotice(
      error instanceof Error
        ? error.message
        : '테스트케이스를 추가하지 못했습니다.'
    )
  } finally {
    busySection.value = ''
  }
}

async function handleDeleteLastTestcase(){
  if (!canDeleteLastTestcase.value || !authState.token) {
    return
  }

  busySection.value = 'delete-last'

  try {
    await deleteProblemTestcase(selectedProblemId.value, authState.token)
    testcaseItems.value = testcaseItems.value.slice(0, -1)
    syncSelectedTestcase(selectedTestcaseOrder.value)
    showSuccessNotice('마지막 테스트케이스를 삭제했습니다.')
  } catch (error) {
    showErrorNotice(
      error instanceof Error
        ? error.message
        : '테스트케이스를 삭제하지 못했습니다.'
    )
  } finally {
    busySection.value = ''
  }
}

async function handleSaveSelectedTestcase(){
  if (!selectedTestcase.value || !canSaveSelectedTestcase.value || !authState.token) {
    return
  }

  busySection.value = 'save'

  const testcaseOrder = selectedTestcase.value.testcase_order
  const nextTestcaseInput = selectedTestcaseInputDraft.value
  const nextTestcaseOutput = selectedTestcaseOutputDraft.value

  try {
    const response = await updateProblemTestcase(
      selectedProblemId.value,
      testcaseOrder,
      {
        testcase_input: nextTestcaseInput,
        testcase_output: nextTestcaseOutput
      },
      authState.token
    )

    testcaseItems.value = testcaseItems.value.map((testcase) => {
      if (testcase.testcase_order !== testcaseOrder) {
        return testcase
      }

      return {
        ...testcase,
        testcase_id: Number(response.testcase_id ?? testcase.testcase_id),
        testcase_order: Number(response.testcase_order ?? testcase.testcase_order),
        testcase_input: typeof response.testcase_input === 'string'
          ? response.testcase_input
          : nextTestcaseInput,
        testcase_output: typeof response.testcase_output === 'string'
          ? response.testcase_output
          : nextTestcaseOutput
      }
    })
    showSuccessNotice(`테스트케이스 ${testcaseOrder}번을 저장했습니다.`)
  } catch (error) {
    showErrorNotice(
      error instanceof Error
        ? error.message
        : '테스트케이스를 저장하지 못했습니다.'
    )
  } finally {
    busySection.value = ''
  }
}

onMounted(async () => {
  await initializeAuth()

  if (!canManageProblems.value) {
    isLoadingProblems.value = false
    isLoadingProblem.value = false
    isLoadingTestcases.value = false
    return
  }

  await loadProblems()
  await loadSelectedProblemData()
})
</script>

<style scoped>
.admin-testcases-shell {
  display: grid;
  gap: 1.25rem;
}

.admin-testcases-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
  flex-wrap: wrap;
}

.admin-testcases-copy,
.admin-testcases-problem-list-caption,
.admin-testcases-problem-list-count,
.admin-testcases-editor-copy {
  margin: 0;
  color: var(--ink-soft);
}

.admin-testcases-copy {
  margin-top: 0.55rem;
  max-width: 68ch;
}

.admin-testcases-toolbar-actions {
  display: flex;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
}

.admin-testcases-layout {
  display: grid;
  grid-template-columns: minmax(320px, 380px) minmax(0, 1fr);
  gap: 1rem;
  align-items: start;
}

.admin-testcases-problem-list-panel,
.admin-testcases-editor-panel {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border: 1px solid var(--line);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.7);
}

.admin-testcases-editor-panel {
  min-height: 40rem;
}

.admin-testcases-problem-search {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-testcases-problem-search-input {
  flex: 1 1 220px;
}

.admin-testcases-problem-list-header {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-testcases-problem-list-count {
  font-size: 0.92rem;
  font-weight: 700;
}

.admin-testcases-problem-list {
  display: grid;
  gap: 0.75rem;
  max-height: 65vh;
  overflow-y: auto;
  padding: 0.15rem 0.2rem 0.15rem 0.05rem;
}

.admin-testcases-problem-item {
  appearance: none;
  width: 100%;
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  text-align: left;
  font: inherit;
  color: var(--ink-strong);
  cursor: pointer;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    background 160ms ease,
    box-shadow 160ms ease;
}

.admin-testcases-problem-item:hover {
  transform: translateY(-1px);
}

.admin-testcases-problem-item:hover,
.admin-testcases-problem-item.is-active {
  border-color: rgba(217, 119, 6, 0.32);
  background: rgba(255, 251, 235, 0.98);
  box-shadow: 0 12px 28px rgba(217, 119, 6, 0.12);
}

.admin-testcases-problem-item-head,
.admin-testcases-problem-item-limits {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-testcases-problem-item-head {
  color: var(--ink-soft);
  font-size: 0.86rem;
}

.admin-testcases-problem-item-title {
  display: block;
  font-size: 1rem;
  line-height: 1.45;
}

.admin-testcases-problem-item-version,
.admin-testcases-problem-item-limits {
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 600;
}

.admin-testcases-editor-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-testcases-editor-header h3 {
  margin: 0;
}

.admin-testcases-editor-side {
  display: grid;
  gap: 0.7rem;
  justify-items: end;
  flex: 0 0 auto;
}

.admin-testcases-editor-current-problem {
  margin: 0;
  color: var(--ink-soft);
  text-align: right;
  font-weight: 600;
  line-height: 1.4;
}

.admin-testcases-editor-actions {
  display: flex;
  gap: 0.75rem;
  flex-wrap: wrap;
  justify-content: flex-end;
}

.admin-testcases-section,
.admin-testcase-card {
  border: 1px solid var(--line);
  border-radius: 20px;
  background: var(--surface-strong);
}

.admin-testcases-section {
  padding: 1.2rem;
}

.admin-testcase-card {
  padding: 1rem;
}

.admin-testcases-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 1rem;
}

.admin-testcases-textarea {
  width: 100%;
  min-height: 12rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
  resize: vertical;
  line-height: 1.6;
}

.admin-testcases-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-testcases-actions {
  display: flex;
  justify-content: flex-end;
  margin-top: 1rem;
}

.admin-testcases-order-form {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: end;
  margin-bottom: 1rem;
  flex-wrap: wrap;
}

.admin-testcases-order-field {
  flex: 1 1 16rem;
}

.admin-testcases-order-actions {
  display: flex;
  gap: 0.75rem;
  flex-wrap: wrap;
  align-items: center;
}

.admin-testcases-summary-layout {
  display: grid;
  grid-template-columns: minmax(260px, 320px) minmax(0, 1fr);
  gap: 1rem;
  align-items: start;
}

.admin-testcases-summary-panel {
  padding: 0.85rem;
  border: 1px solid var(--line);
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.88);
}

.admin-testcases-summary-list {
  display: grid;
  gap: 0.75rem;
  max-height: 40rem;
  overflow: auto;
  padding: 0.15rem 0.2rem 0.15rem 0.05rem;
}

.admin-testcase-summary-item {
  appearance: none;
  display: grid;
  gap: 0.25rem;
  width: 100%;
  padding: 0.9rem 1rem;
  border: 1px solid var(--line);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.82);
  color: var(--ink-strong);
  text-align: left;
  font: inherit;
  cursor: pointer;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    background 160ms ease;
}

.admin-testcase-summary-item:hover {
  transform: translateY(-1px);
}

.admin-testcase-summary-item:hover,
.admin-testcase-summary-item.is-active {
  border-color: rgba(217, 119, 6, 0.34);
  background: rgba(255, 247, 237, 0.96);
  box-shadow: 0 10px 24px rgba(217, 119, 6, 0.1);
}

.admin-testcase-summary-head {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-testcase-summary-badge {
  padding: 0.15rem 0.5rem;
  border-radius: 999px;
  background: rgba(217, 119, 6, 0.12);
  color: var(--warning);
  font-size: 0.78rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.06em;
}

.admin-testcase-summary-copy {
  color: var(--ink-soft);
  font-size: 0.92rem;
}

.admin-testcase-card-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
  margin-bottom: 1rem;
}

.admin-testcase-delete-button {
  flex: 0 0 auto;
}

@media (max-width: 1100px) {
  .admin-testcases-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-testcases-problem-list {
    max-height: 24rem;
  }
}

@media (max-width: 720px) {
  .admin-testcases-problem-search,
  .admin-testcases-order-form {
    display: grid;
    align-items: stretch;
  }

  .admin-testcases-grid {
    grid-template-columns: 1fr;
  }

  .admin-testcases-summary-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-testcases-problem-item-head,
  .admin-testcases-problem-item-limits,
  .admin-testcase-card-header,
  .admin-testcases-editor-header {
    align-items: start;
    flex-direction: column;
  }

  .admin-testcases-editor-side {
    width: 100%;
    justify-items: start;
  }

  .admin-testcases-editor-current-problem,
  .admin-testcases-editor-actions {
    text-align: left;
    justify-content: flex-start;
  }

  .admin-testcases-actions {
    justify-content: stretch;
  }
}
</style>
