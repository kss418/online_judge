<template>
  <section class="page-grid single-column">
    <article class="panel admin-problems-panel">
      <div class="admin-problems-toolbar">
        <div>
          <p class="panel-kicker">Problem Admin</p>
          <h3>문제 관리</h3>
          <p class="admin-problems-copy">
            문제를 생성하고 제목, 제한, 설명, 삭제를 한 화면에서 관리할 수 있습니다.
          </p>
        </div>

        <div class="admin-problems-actions">
          <StatusBadge
            :label="isLoadingProblems ? 'Loading' : `${problemCount} Problems`"
            :tone="listErrorMessage ? 'danger' : 'success'"
          />
          <button
            v-if="canManageProblems"
            type="button"
            class="ghost-button"
            :disabled="isLoadingProblems || Boolean(busySection)"
            @click="loadProblems({ preferredProblemId: selectedProblemId })"
          >
            새로고침
          </button>
        </div>
      </div>

      <div v-if="authState.isInitializing" class="empty-state">
        <p>관리자 권한을 확인하는 중입니다.</p>
      </div>

      <div v-else-if="!isAuthenticated" class="empty-state">
        <p>문제 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.</p>
      </div>

      <div v-else-if="!canManageProblems" class="empty-state error-state">
        <p>이 페이지는 관리자만 접근할 수 있습니다.</p>
      </div>

      <template v-else>
        <div v-if="actionMessage" class="admin-problems-feedback is-success">
          <p>{{ actionMessage }}</p>
        </div>

        <div v-if="actionErrorMessage" class="admin-problems-feedback is-error">
          <p>{{ actionErrorMessage }}</p>
        </div>

        <div class="admin-problems-layout">
          <aside class="admin-problem-list-panel">
            <form class="admin-problem-search" @submit.prevent="submitSearch">
              <label class="sr-only" for="admin-problem-title-search">문제 검색</label>
              <input
                id="admin-problem-title-search"
                v-model="searchInput"
                class="field-input admin-problem-search-input"
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

            <form class="admin-problem-create" @submit.prevent="handleCreateProblem">
              <div>
                <p class="panel-kicker">create</p>
                <h3>새 문제</h3>
              </div>
              <div class="admin-problem-create-row">
                <input
                  v-model.trim="newProblemTitle"
                  class="field-input"
                  type="text"
                  maxlength="120"
                  placeholder="새 문제 제목"
                />
                <button
                  type="submit"
                  class="primary-button"
                  :disabled="!canCreateProblem"
                >
                  {{ isCreatingProblem ? '생성 중...' : '문제 생성' }}
                </button>
              </div>
            </form>

            <div class="admin-problem-list-header">
              <p class="admin-problem-list-caption">
                {{ searchKeyword ? `"${searchKeyword}" 검색 결과` : '전체 문제' }}
              </p>
              <span class="admin-problem-list-count">{{ formatCount(problemCount) }}개</span>
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

            <div v-else class="admin-problem-list">
              <button
                v-for="problem in problems"
                :key="problem.problem_id"
                type="button"
                class="admin-problem-item"
                :class="{ 'is-active': problem.problem_id === selectedProblemId }"
                @click="selectProblem(problem.problem_id)"
              >
                <div class="admin-problem-item-head">
                  <strong>#{{ formatCount(problem.problem_id) }}</strong>
                  <span class="admin-problem-item-version">v{{ problem.version }}</span>
                </div>
                <strong class="admin-problem-item-title">{{ problem.title }}</strong>
                <div class="admin-problem-item-limits">
                  <span>{{ formatProblemLimit(problem.time_limit_ms, 'ms') }}</span>
                  <span>{{ formatProblemLimit(problem.memory_limit_mb, 'MB') }}</span>
                </div>
              </button>
            </div>
          </aside>

          <section class="admin-problem-editor-panel">
            <div v-if="isLoadingDetail" class="empty-state">
              <p>문제 정보를 불러오는 중입니다.</p>
            </div>

            <div v-else-if="detailErrorMessage" class="empty-state error-state">
              <p>{{ detailErrorMessage }}</p>
            </div>

            <div v-else-if="!selectedProblemDetail" class="empty-state">
              <p>왼쪽 목록에서 문제를 선택하거나 새 문제를 생성하세요.</p>
            </div>

            <template v-else>
              <div class="admin-problem-editor-header">
                <div>
                  <p class="panel-kicker">editor</p>
                  <h3>#{{ formatCount(selectedProblemDetail.problem_id) }} {{ selectedProblemDetail.title }}</h3>
                  <p class="admin-problem-editor-copy">
                    선택한 문제의 제목, 제한, 설명을 바로 저장할 수 있습니다.
                  </p>
                </div>

                <div class="admin-problem-editor-header-actions">
                  <button
                    type="button"
                    class="ghost-button admin-problem-rejudge-button"
                    :disabled="Boolean(busySection)"
                    @click="openRejudgeDialog"
                  >
                    재채점
                  </button>
                  <RouterLink
                    class="ghost-button"
                    :to="{ name: 'problem-detail', params: { problemId: selectedProblemDetail.problem_id } }"
                  >
                    문제 보기
                  </RouterLink>
                </div>
              </div>

              <article class="admin-problem-editor-section">
                <div class="panel-header">
                  <div>
                    <p class="panel-kicker">title</p>
                    <h3>제목</h3>
                  </div>
                </div>

                <div class="admin-problem-form-grid single-column-grid">
                  <input
                    v-model="titleDraft"
                    class="field-input"
                    type="text"
                    maxlength="120"
                    placeholder="문제 제목"
                  />
                </div>

                <div class="admin-problem-section-actions">
                  <button
                    type="button"
                    class="primary-button"
                    :disabled="!canSaveTitle"
                    @click="handleSaveTitle"
                  >
                    {{ isSavingTitle ? '저장 중...' : '제목 저장' }}
                  </button>
                </div>
              </article>

              <article class="admin-problem-editor-section">
                <div class="panel-header">
                  <div>
                    <p class="panel-kicker">limits</p>
                    <h3>실행 제한</h3>
                  </div>
                </div>

                <div class="admin-problem-form-grid">
                  <label class="field-block">
                    <span class="field-label">시간 제한(ms)</span>
                    <input
                      v-model="timeLimitDraft"
                      class="field-input"
                      type="number"
                      min="1"
                      inputmode="numeric"
                      placeholder="1000"
                    />
                  </label>

                  <label class="field-block">
                    <span class="field-label">메모리 제한(MB)</span>
                    <input
                      v-model="memoryLimitDraft"
                      class="field-input"
                      type="number"
                      min="1"
                      inputmode="numeric"
                      placeholder="256"
                    />
                  </label>
                </div>

                <div class="admin-problem-section-actions">
                  <button
                    type="button"
                    class="primary-button"
                    :disabled="!canSaveLimits"
                    @click="handleSaveLimits"
                  >
                    {{ isSavingLimits ? '저장 중...' : '제한 저장' }}
                  </button>
                </div>
              </article>

              <article class="admin-problem-editor-section">
                <div class="panel-header">
                  <div>
                    <p class="panel-kicker">statement</p>
                    <h3>문제 설명</h3>
                  </div>
                </div>

                <div class="admin-problem-form-grid single-column-grid">
                  <label class="field-block">
                    <span class="field-label">문제 설명</span>
                    <textarea
                      v-model="descriptionDraft"
                      class="admin-problem-textarea"
                      spellcheck="false"
                    />
                  </label>

                  <label class="field-block">
                    <span class="field-label">입력 설명</span>
                    <textarea
                      v-model="inputFormatDraft"
                      class="admin-problem-textarea"
                      spellcheck="false"
                    />
                  </label>

                  <label class="field-block">
                    <span class="field-label">출력 설명</span>
                    <textarea
                      v-model="outputFormatDraft"
                      class="admin-problem-textarea"
                      spellcheck="false"
                    />
                  </label>

                  <label class="field-block">
                    <span class="field-label">비고</span>
                    <textarea
                      v-model="noteDraft"
                      class="admin-problem-textarea is-note"
                      spellcheck="false"
                      placeholder="없으면 비워 두세요."
                    />
                  </label>
                </div>

                <div class="admin-problem-section-actions">
                  <button
                    type="button"
                    class="primary-button"
                    :disabled="!canSaveStatement"
                    @click="handleSaveStatement"
                  >
                    {{ isSavingStatement ? '저장 중...' : '설명 저장' }}
                  </button>
                </div>
              </article>

              <article class="admin-problem-editor-section is-danger">
                <div class="panel-header">
                  <div>
                    <p class="panel-kicker">danger zone</p>
                    <h3>문제 삭제</h3>
                  </div>
                </div>

                <p class="admin-problem-danger-copy">
                  삭제 전에 문제 번호와 제목을 다시 입력해야 합니다. 제출이 연결된 문제는 삭제가 거부될 수 있습니다.
                </p>

                <div class="admin-problem-section-actions">
                  <button
                    type="button"
                    class="ghost-button admin-problem-danger-button"
                    :disabled="Boolean(busySection)"
                    @click="openDeleteDialog"
                  >
                    문제 삭제
                  </button>
                </div>
              </article>
            </template>
          </section>
        </div>
      </template>
    </article>
  </section>

  <Teleport to="body">
    <div
      v-if="rejudgeDialogOpen"
      class="admin-problem-delete-backdrop"
      @pointerdown="handleRejudgeBackdropPointerDown"
      @click.self="handleRejudgeBackdropClick"
    >
      <section
        class="admin-problem-delete-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="admin-problem-rejudge-title"
      >
        <div class="admin-problem-delete-header">
          <div>
            <p class="panel-kicker">confirm rejudge</p>
            <h3 id="admin-problem-rejudge-title">문제 재채점 확인</h3>
            <p class="auth-dialog-copy">
              아래 두 값을 모두 정확히 다시 입력해야 재채점을 요청할 수 있습니다.
            </p>
          </div>
          <button
            type="button"
            class="icon-button"
            aria-label="닫기"
            :disabled="isRejudgingProblem"
            @click="closeRejudgeDialog"
          >
            ×
          </button>
        </div>

        <div v-if="selectedProblemDetail" class="admin-problem-delete-summary">
          <strong>#{{ formatCount(selectedProblemDetail.problem_id) }} {{ selectedProblemDetail.title }}</strong>
          <p class="admin-problem-confirm-copy">
            현재 문제의 `accepted`, `wrong_answer` 제출을 다시 채점 대기열에 넣습니다.
          </p>
        </div>

        <div class="field-block">
          <label class="field-label" for="rejudge-problem-id-confirm">문제 번호 다시 입력</label>
          <input
            id="rejudge-problem-id-confirm"
            v-model="rejudgeConfirmProblemId"
            class="field-input"
            type="text"
            inputmode="numeric"
            :disabled="isRejudgingProblem"
            placeholder="예: 1000"
          />
        </div>

        <div class="field-block">
          <label class="field-label" for="rejudge-problem-title-confirm">문제 제목 다시 입력</label>
          <input
            id="rejudge-problem-title-confirm"
            v-model="rejudgeConfirmTitle"
            class="field-input"
            type="text"
            :disabled="isRejudgingProblem"
            placeholder="문제 제목"
          />
        </div>

        <div class="dialog-actions">
          <button
            type="button"
            class="ghost-button"
            :disabled="isRejudgingProblem"
            @click="closeRejudgeDialog"
          >
            취소
          </button>
          <button
            type="button"
            class="primary-button admin-problem-rejudge-confirm"
            :disabled="!canRejudgeSelectedProblem"
            @click="handleRejudgeProblem"
          >
            {{ isRejudgingProblem ? '요청 중...' : '재채점 확정' }}
          </button>
        </div>
      </section>
    </div>
  </Teleport>

  <Teleport to="body">
    <div
      v-if="deleteDialogOpen"
      class="admin-problem-delete-backdrop"
      @pointerdown="handleDeleteBackdropPointerDown"
      @click.self="handleDeleteBackdropClick"
    >
      <section
        class="admin-problem-delete-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="admin-problem-delete-title"
      >
        <div class="admin-problem-delete-header">
          <div>
            <p class="panel-kicker">confirm delete</p>
            <h3 id="admin-problem-delete-title">문제 삭제 확인</h3>
            <p class="auth-dialog-copy">
              아래 두 값을 모두 정확히 다시 입력해야 삭제할 수 있습니다.
            </p>
          </div>
          <button
            type="button"
            class="icon-button"
            aria-label="닫기"
            :disabled="isDeletingProblem"
            @click="closeDeleteDialog"
          >
            ×
          </button>
        </div>

        <div v-if="selectedProblemDetail" class="admin-problem-delete-summary">
          <strong>#{{ formatCount(selectedProblemDetail.problem_id) }} {{ selectedProblemDetail.title }}</strong>
        </div>

        <div class="field-block">
          <label class="field-label" for="delete-problem-id-confirm">문제 번호 다시 입력</label>
          <input
            id="delete-problem-id-confirm"
            v-model="deleteConfirmProblemId"
            class="field-input"
            type="text"
            inputmode="numeric"
            :disabled="isDeletingProblem"
            placeholder="예: 1000"
          />
        </div>

        <div class="field-block">
          <label class="field-label" for="delete-problem-title-confirm">문제 제목 다시 입력</label>
          <input
            id="delete-problem-title-confirm"
            v-model="deleteConfirmTitle"
            class="field-input"
            type="text"
            :disabled="isDeletingProblem"
            placeholder="문제 제목"
          />
        </div>

        <div class="dialog-actions">
          <button
            type="button"
            class="ghost-button"
            :disabled="isDeletingProblem"
            @click="closeDeleteDialog"
          >
            취소
          </button>
          <button
            type="button"
            class="primary-button admin-problem-delete-confirm"
            :disabled="!canDeleteSelectedProblem"
            @click="handleDeleteProblem"
          >
            {{ isDeletingProblem ? '삭제 중...' : '삭제 확정' }}
          </button>
        </div>
      </section>
    </div>
  </Teleport>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'

import {
  createProblem,
  deleteProblem,
  getProblemDetail,
  getProblemList,
  rejudgeProblem,
  updateProblemLimits,
  updateProblemStatement,
  updateProblemTitle
} from '@/api/problem'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAuth } from '@/composables/useAuth'

const { authState, isAuthenticated, initializeAuth } = useAuth()
const countFormatter = new Intl.NumberFormat()

const isLoadingProblems = ref(true)
const isLoadingDetail = ref(false)
const listErrorMessage = ref('')
const detailErrorMessage = ref('')
const actionErrorMessage = ref('')
const actionMessage = ref('')
const searchInput = ref('')
const searchKeyword = ref('')
const newProblemTitle = ref('')
const problems = ref([])
const selectedProblemId = ref(0)
const selectedProblemDetail = ref(null)
const titleDraft = ref('')
const timeLimitDraft = ref('')
const memoryLimitDraft = ref('')
const descriptionDraft = ref('')
const inputFormatDraft = ref('')
const outputFormatDraft = ref('')
const noteDraft = ref('')
const busySection = ref('')
const rejudgeDialogOpen = ref(false)
const rejudgeConfirmProblemId = ref('')
const rejudgeConfirmTitle = ref('')
const isRejudgeBackdropInteraction = ref(false)
const deleteDialogOpen = ref(false)
const deleteConfirmProblemId = ref('')
const deleteConfirmTitle = ref('')
const isDeleteBackdropInteraction = ref(false)
let latestProblemListRequestId = 0
let latestProblemDetailRequestId = 0
let latestProblemLimitHydrationId = 0

const canManageProblems = computed(() => Number(authState.currentUser?.permission_level ?? 0) >= 1)
const problemCount = computed(() => problems.value.length)
const canCreateProblem = computed(() =>
  Boolean(authState.token) &&
  canManageProblems.value &&
  !busySection.value &&
  Boolean(newProblemTitle.value.trim())
)
const isCreatingProblem = computed(() => busySection.value === 'create')
const isSavingTitle = computed(() => busySection.value === 'title')
const isSavingLimits = computed(() => busySection.value === 'limits')
const isSavingStatement = computed(() => busySection.value === 'statement')
const isRejudgingProblem = computed(() => busySection.value === 'rejudge')
const isDeletingProblem = computed(() => busySection.value === 'delete')
const canSaveTitle = computed(() => {
  if (!selectedProblemDetail.value || !authState.token || busySection.value) {
    return false
  }

  const nextTitle = titleDraft.value.trim()
  return Boolean(nextTitle) && nextTitle !== selectedProblemDetail.value.title
})
const canSaveLimits = computed(() => {
  if (!selectedProblemDetail.value || !authState.token || busySection.value) {
    return false
  }

  const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
  const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)

  if (nextTimeLimit == null || nextMemoryLimit == null) {
    return false
  }

  return (
    nextTimeLimit !== selectedProblemDetail.value.limits.time_limit_ms ||
    nextMemoryLimit !== selectedProblemDetail.value.limits.memory_limit_mb
  )
})
const canSaveStatement = computed(() => {
  if (!selectedProblemDetail.value || !authState.token || busySection.value) {
    return false
  }

  if (
    !descriptionDraft.value.trim() ||
    !inputFormatDraft.value.trim() ||
    !outputFormatDraft.value.trim()
  ) {
    return false
  }

  return (
    descriptionDraft.value !== selectedProblemDetail.value.statement.description ||
    inputFormatDraft.value !== selectedProblemDetail.value.statement.input_format ||
    outputFormatDraft.value !== selectedProblemDetail.value.statement.output_format ||
    noteDraft.value !== selectedProblemDetail.value.statement.note
  )
})
const canDeleteSelectedProblem = computed(() => {
  if (!selectedProblemDetail.value || !deleteDialogOpen.value || busySection.value) {
    return false
  }

  const expectedProblemId = String(selectedProblemDetail.value.problem_id)
  return (
    deleteConfirmProblemId.value.trim() === expectedProblemId &&
    deleteConfirmTitle.value === selectedProblemDetail.value.title
  )
})
const canRejudgeSelectedProblem = computed(() => {
  if (!selectedProblemDetail.value || !rejudgeDialogOpen.value || busySection.value) {
    return false
  }

  const expectedProblemId = String(selectedProblemDetail.value.problem_id)
  return (
    rejudgeConfirmProblemId.value.trim() === expectedProblemId &&
    rejudgeConfirmTitle.value === selectedProblemDetail.value.title
  )
})

watch(
  () => [authState.initialized, authState.token, authState.currentUser?.permission_level],
  () => {
    if (!authState.initialized) {
      isLoadingProblems.value = true
      return
    }

    if (!isAuthenticated.value || !canManageProblems.value) {
      resetPageState()
      isLoadingProblems.value = false
      return
    }

    void loadProblems({ preferredProblemId: selectedProblemId.value })
  },
  { immediate: true }
)

function resetPageState(){
  searchInput.value = ''
  searchKeyword.value = ''
  newProblemTitle.value = ''
  problems.value = []
  selectedProblemId.value = 0
  selectedProblemDetail.value = null
  listErrorMessage.value = ''
  detailErrorMessage.value = ''
  actionErrorMessage.value = ''
  actionMessage.value = ''
  busySection.value = ''
  closeRejudgeDialog(true)
  closeDeleteDialog(true)
  resetEditorDrafts()
}

function resetEditorDrafts(){
  titleDraft.value = ''
  timeLimitDraft.value = ''
  memoryLimitDraft.value = ''
  descriptionDraft.value = ''
  inputFormatDraft.value = ''
  outputFormatDraft.value = ''
  noteDraft.value = ''
}

function formatCount(value){
  return countFormatter.format(Number(value ?? 0))
}

function formatProblemLimit(value, unit){
  const numericValue = Number(value)

  if (!Number.isFinite(numericValue) || numericValue <= 0) {
    return unit === 'ms' ? '시간 확인 중' : '메모리 확인 중'
  }

  return `${formatCount(numericValue)} ${unit}`
}

function normalizeProblemDetail(response){
  return {
    problem_id: Number(response.problem_id ?? 0),
    title: response.title ?? '',
    version: Number(response.version ?? 0),
    limits: {
      time_limit_ms: Number(response.limits?.time_limit_ms ?? 0),
      memory_limit_mb: Number(response.limits?.memory_limit_mb ?? 0)
    },
    statement: {
      description: response.statement?.description ?? '',
      input_format: response.statement?.input_format ?? '',
      output_format: response.statement?.output_format ?? '',
      note: response.statement?.note ?? ''
    }
  }
}

function assignEditorDrafts(problemDetail){
  titleDraft.value = problemDetail.title
  timeLimitDraft.value = String(problemDetail.limits.time_limit_ms || '')
  memoryLimitDraft.value = String(problemDetail.limits.memory_limit_mb || '')
  descriptionDraft.value = problemDetail.statement.description
  inputFormatDraft.value = problemDetail.statement.input_format
  outputFormatDraft.value = problemDetail.statement.output_format
  noteDraft.value = problemDetail.statement.note
}

function mergeProblemSummary(problemId, patch){
  problems.value = problems.value.map((problem) =>
    problem.problem_id === problemId
      ? {
        ...problem,
        ...patch
      }
      : problem
  )
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
          time_limit_ms: detail.limits.time_limit_ms,
          memory_limit_mb: detail.limits.memory_limit_mb
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

async function loadProblems(options = {}){
  if (!authState.token || !canManageProblems.value) {
    return
  }

  const requestId = ++latestProblemListRequestId
  const preferredProblemId = Number(options.preferredProblemId ?? selectedProblemId.value)

  isLoadingProblems.value = true
  listErrorMessage.value = ''
  actionErrorMessage.value = ''

  try {
    const response = await getProblemList({
      title: searchKeyword.value,
      bearerToken: authState.token
    })

    if (requestId !== latestProblemListRequestId) {
      return
    }

    const responseProblems = Array.isArray(response.problems) ? response.problems : []
    problems.value = responseProblems.map((problem) => ({
      problem_id: Number(problem.problem_id ?? 0),
      title: problem.title ?? '',
      version: Number(problem.version ?? 0),
      time_limit_ms: null,
      memory_limit_mb: null
    }))

    if (!problems.value.length) {
      selectedProblemId.value = 0
      selectedProblemDetail.value = null
      detailErrorMessage.value = ''
      resetEditorDrafts()
      return
    }

    void hydrateProblemLimits(problems.value.map((problem) => problem.problem_id))

    const nextProblemId = problems.value.some((problem) => problem.problem_id === preferredProblemId)
      ? preferredProblemId
      : problems.value[0].problem_id

    void selectProblem(nextProblemId, {
      force: nextProblemId !== selectedProblemId.value || !selectedProblemDetail.value
    })
  } catch (error) {
    if (requestId !== latestProblemListRequestId) {
      return
    }

    problems.value = []
    selectedProblemId.value = 0
    selectedProblemDetail.value = null
    resetEditorDrafts()
    listErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 목록을 불러오지 못했습니다.'
  } finally {
    if (requestId === latestProblemListRequestId) {
      isLoadingProblems.value = false
    }
  }
}

async function loadSelectedProblem(problemId){
  if (!problemId) {
    return
  }

  const requestId = ++latestProblemDetailRequestId
  selectedProblemId.value = problemId
  isLoadingDetail.value = true
  detailErrorMessage.value = ''
  actionErrorMessage.value = ''
  selectedProblemDetail.value = null
  resetEditorDrafts()

  try {
    const response = await getProblemDetail(problemId, {
      bearerToken: authState.token || ''
    })

    if (requestId !== latestProblemDetailRequestId) {
      return
    }

    const normalizedDetail = normalizeProblemDetail(response)
    selectedProblemDetail.value = normalizedDetail
    mergeProblemSummary(problemId, {
      title: normalizedDetail.title,
      version: normalizedDetail.version,
      time_limit_ms: normalizedDetail.limits.time_limit_ms,
      memory_limit_mb: normalizedDetail.limits.memory_limit_mb
    })
    assignEditorDrafts(normalizedDetail)
  } catch (error) {
    if (requestId !== latestProblemDetailRequestId) {
      return
    }

    detailErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 정보를 불러오지 못했습니다.'
  } finally {
    if (requestId === latestProblemDetailRequestId) {
      isLoadingDetail.value = false
    }
  }
}

async function selectProblem(problemId, options = {}){
  if (!problemId) {
    return
  }

  if (!options.force && selectedProblemId.value === problemId && selectedProblemDetail.value) {
    return
  }

  await loadSelectedProblem(problemId)
}

function submitSearch(){
  searchKeyword.value = searchInput.value.trim()
  void loadProblems({ preferredProblemId: selectedProblemId.value })
}

function resetSearch(){
  searchInput.value = ''
  searchKeyword.value = ''
  void loadProblems({ preferredProblemId: selectedProblemId.value })
}

async function handleCreateProblem(){
  if (!authState.token || !canCreateProblem.value) {
    return
  }

  busySection.value = 'create'
  actionErrorMessage.value = ''
  actionMessage.value = ''

  try {
    const response = await createProblem({
      title: newProblemTitle.value.trim()
    }, authState.token)

    newProblemTitle.value = ''
    searchInput.value = ''
    searchKeyword.value = ''
    actionMessage.value = `문제 #${formatCount(response.problem_id)}를 생성했습니다.`

    await loadProblems({
      preferredProblemId: Number(response.problem_id ?? 0)
    })
  } catch (error) {
    actionErrorMessage.value = error instanceof Error
      ? error.message
      : '문제를 생성하지 못했습니다.'
  } finally {
    busySection.value = ''
  }
}

async function handleSaveTitle(){
  if (!authState.token || !selectedProblemDetail.value || !canSaveTitle.value) {
    return
  }

  busySection.value = 'title'
  actionErrorMessage.value = ''
  actionMessage.value = ''

  try {
    const nextTitle = titleDraft.value.trim()
    await updateProblemTitle(selectedProblemDetail.value.problem_id, {
      title: nextTitle
    }, authState.token)

    selectedProblemDetail.value = {
      ...selectedProblemDetail.value,
      title: nextTitle
    }
    titleDraft.value = nextTitle
    mergeProblemSummary(selectedProblemDetail.value.problem_id, {
      title: nextTitle
    })
    actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 제목을 저장했습니다.`
  } catch (error) {
    actionErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 제목을 저장하지 못했습니다.'
  } finally {
    busySection.value = ''
  }
}

async function handleSaveLimits(){
  if (!authState.token || !selectedProblemDetail.value || !canSaveLimits.value) {
    return
  }

  const nextTimeLimit = parsePositiveInteger(timeLimitDraft.value)
  const nextMemoryLimit = parsePositiveInteger(memoryLimitDraft.value)
  if (nextTimeLimit == null || nextMemoryLimit == null) {
    return
  }

  busySection.value = 'limits'
  actionErrorMessage.value = ''
  actionMessage.value = ''

  try {
    await updateProblemLimits(selectedProblemDetail.value.problem_id, {
      time_limit_ms: nextTimeLimit,
      memory_limit_mb: nextMemoryLimit
    }, authState.token)

    selectedProblemDetail.value = {
      ...selectedProblemDetail.value,
      limits: {
        time_limit_ms: nextTimeLimit,
        memory_limit_mb: nextMemoryLimit
      }
    }
    timeLimitDraft.value = String(nextTimeLimit)
    memoryLimitDraft.value = String(nextMemoryLimit)
    mergeProblemSummary(selectedProblemDetail.value.problem_id, {
      time_limit_ms: nextTimeLimit,
      memory_limit_mb: nextMemoryLimit
    })
    actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 제한을 저장했습니다.`
  } catch (error) {
    actionErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 제한을 저장하지 못했습니다.'
  } finally {
    busySection.value = ''
  }
}

async function handleSaveStatement(){
  if (!authState.token || !selectedProblemDetail.value || !canSaveStatement.value) {
    return
  }

  busySection.value = 'statement'
  actionErrorMessage.value = ''
  actionMessage.value = ''

  try {
    const nextStatement = {
      description: descriptionDraft.value,
      input_format: inputFormatDraft.value,
      output_format: outputFormatDraft.value,
      note: noteDraft.value === '' ? null : noteDraft.value
    }

    await updateProblemStatement(selectedProblemDetail.value.problem_id, nextStatement, authState.token)

    selectedProblemDetail.value = {
      ...selectedProblemDetail.value,
      statement: {
        description: descriptionDraft.value,
        input_format: inputFormatDraft.value,
        output_format: outputFormatDraft.value,
        note: noteDraft.value
      }
    }
    actionMessage.value = `문제 #${formatCount(selectedProblemDetail.value.problem_id)} 설명을 저장했습니다.`
  } catch (error) {
    actionErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 설명을 저장하지 못했습니다.'
  } finally {
    busySection.value = ''
  }
}

function openDeleteDialog(){
  if (!selectedProblemDetail.value || busySection.value) {
    return
  }

  deleteConfirmProblemId.value = ''
  deleteConfirmTitle.value = ''
  isDeleteBackdropInteraction.value = false
  deleteDialogOpen.value = true
}

function openRejudgeDialog(){
  if (!selectedProblemDetail.value || busySection.value) {
    return
  }

  rejudgeConfirmProblemId.value = ''
  rejudgeConfirmTitle.value = ''
  isRejudgeBackdropInteraction.value = false
  rejudgeDialogOpen.value = true
}

function closeDeleteDialog(force = false){
  if (!force && isDeletingProblem.value) {
    return
  }

  deleteDialogOpen.value = false
  isDeleteBackdropInteraction.value = false
  deleteConfirmProblemId.value = ''
  deleteConfirmTitle.value = ''
}

function closeRejudgeDialog(force = false){
  if (!force && isRejudgingProblem.value) {
    return
  }

  rejudgeDialogOpen.value = false
  isRejudgeBackdropInteraction.value = false
  rejudgeConfirmProblemId.value = ''
  rejudgeConfirmTitle.value = ''
}

function handleDeleteBackdropPointerDown(event){
  isDeleteBackdropInteraction.value = event.target === event.currentTarget
}

function handleRejudgeBackdropPointerDown(event){
  isRejudgeBackdropInteraction.value = event.target === event.currentTarget
}

function handleDeleteBackdropClick(){
  if (!isDeleteBackdropInteraction.value) {
    return
  }

  closeDeleteDialog()
}

function handleRejudgeBackdropClick(){
  if (!isRejudgeBackdropInteraction.value) {
    return
  }

  closeRejudgeDialog()
}

async function handleRejudgeProblem(){
  if (!authState.token || !selectedProblemDetail.value || !canRejudgeSelectedProblem.value) {
    return
  }

  const rejudgingProblemId = selectedProblemDetail.value.problem_id
  busySection.value = 'rejudge'
  actionErrorMessage.value = ''
  actionMessage.value = ''

  try {
    await rejudgeProblem(rejudgingProblemId, authState.token)
    closeRejudgeDialog(true)
    actionMessage.value = `문제 #${formatCount(rejudgingProblemId)} 재채점을 요청했습니다.`
  } catch (error) {
    actionErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 재채점을 요청하지 못했습니다.'
  } finally {
    busySection.value = ''
  }
}

async function handleDeleteProblem(){
  if (!authState.token || !selectedProblemDetail.value || !canDeleteSelectedProblem.value) {
    return
  }

  const deletingProblemId = selectedProblemDetail.value.problem_id
  busySection.value = 'delete'
  actionErrorMessage.value = ''
  actionMessage.value = ''

  try {
    await deleteProblem(deletingProblemId, authState.token)
    closeDeleteDialog(true)
    actionMessage.value = `문제 #${formatCount(deletingProblemId)}를 삭제했습니다.`
    await loadProblems()
  } catch (error) {
    actionErrorMessage.value = error instanceof Error
      ? error.message
      : '문제를 삭제하지 못했습니다.'
  } finally {
    busySection.value = ''
  }
}

function parsePositiveInteger(value){
  const numericValue = Number.parseInt(String(value), 10)
  return Number.isInteger(numericValue) && numericValue > 0 ? numericValue : null
}

onMounted(() => {
  initializeAuth()
})
</script>

<style scoped>
.admin-problems-panel {
  display: grid;
  gap: 1rem;
}

.admin-problems-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problems-copy {
  margin: 0.45rem 0 0;
  color: var(--ink-soft);
}

.admin-problems-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  align-items: center;
  justify-content: end;
}

.admin-problems-feedback {
  padding: 1rem;
  border-radius: 18px;
  border: 1px solid transparent;
  background: var(--surface-strong);
}

.admin-problems-feedback p {
  margin: 0;
}

.admin-problems-feedback.is-success {
  color: var(--success);
  background: var(--success-soft);
  border-color: rgba(15, 118, 110, 0.16);
}

.admin-problems-feedback.is-error {
  color: var(--danger);
  background: var(--danger-soft);
  border-color: rgba(185, 28, 28, 0.18);
}

.admin-problems-layout {
  display: grid;
  grid-template-columns: minmax(320px, 380px) minmax(0, 1fr);
  gap: 1rem;
  align-items: start;
}

.admin-problem-list-panel,
.admin-problem-editor-panel {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border: 1px solid var(--line);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.7);
}

.admin-problem-search {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-search-input {
  flex: 1 1 220px;
}

.admin-problem-create {
  display: grid;
  gap: 0.9rem;
  padding: 1rem;
  border-radius: 20px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.84);
}

.admin-problem-create-row {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-create-row .field-input {
  flex: 1 1 220px;
}

.admin-problem-list-header {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-problem-list-caption,
.admin-problem-list-count,
.admin-problem-editor-copy,
.admin-problem-danger-copy {
  margin: 0;
  color: var(--ink-soft);
}

.admin-problem-list-count {
  font-size: 0.92rem;
  font-weight: 700;
}

.admin-problem-list {
  display: grid;
  gap: 0.75rem;
  max-height: 65vh;
  overflow-y: auto;
  padding-right: 0.2rem;
}

.admin-problem-item {
  appearance: none;
  width: 100%;
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  color: inherit;
  text-align: left;
  cursor: pointer;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    background 160ms ease,
    box-shadow 160ms ease;
}

.admin-problem-item:hover,
.admin-problem-item.is-active {
  transform: translateY(-1px);
  border-color: rgba(217, 119, 6, 0.32);
  background: rgba(255, 251, 235, 0.98);
  box-shadow: 0 12px 28px rgba(217, 119, 6, 0.12);
}

.admin-problem-item-head,
.admin-problem-item-limits {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-problem-item-head {
  color: var(--ink-soft);
  font-size: 0.86rem;
}

.admin-problem-item-title {
  display: block;
  font-size: 1rem;
  line-height: 1.45;
}

.admin-problem-item-version,
.admin-problem-item-limits {
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 600;
}

.admin-problem-editor-panel {
  min-height: 40rem;
}

.admin-problem-editor-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-editor-header-actions {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-editor-section {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 22px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.84);
}

.admin-problem-editor-section.is-danger {
  border-color: rgba(185, 28, 28, 0.16);
  background: rgba(255, 250, 250, 0.88);
}

.admin-problem-form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 0.9rem;
}

.admin-problem-form-grid.single-column-grid {
  grid-template-columns: minmax(0, 1fr);
}

.admin-problem-textarea {
  width: 100%;
  min-height: 10rem;
  padding: 1rem;
  resize: vertical;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
  line-height: 1.6;
}

.admin-problem-textarea.is-note {
  min-height: 7rem;
}

.admin-problem-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-problem-section-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-rejudge-button,
.admin-problem-rejudge-confirm,
.admin-problem-danger-button,
.admin-problem-delete-confirm {
  min-width: 8.5rem;
}

.admin-problem-rejudge-button {
  min-width: 6.25rem;
  padding-inline: 0.95rem;
  color: var(--warning);
  background: rgba(255, 247, 237, 0.96);
  border-color: rgba(180, 83, 9, 0.18);
}

.admin-problem-rejudge-confirm {
  background: linear-gradient(135deg, #d97706, #ea580c);
  box-shadow: 0 12px 28px rgba(217, 119, 6, 0.28);
}

.admin-problem-danger-button {
  color: var(--danger);
  background: rgba(254, 242, 242, 0.96);
  border-color: rgba(185, 28, 28, 0.18);
}

.admin-problem-delete-confirm {
  background: linear-gradient(135deg, #dc2626, #b91c1c);
  box-shadow: 0 12px 28px rgba(185, 28, 28, 0.24);
}

.admin-problem-delete-backdrop {
  position: fixed;
  inset: 0;
  z-index: 50;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 1.2rem;
  background: rgba(15, 23, 42, 0.46);
  backdrop-filter: blur(10px);
}

.admin-problem-delete-dialog {
  width: min(100%, 560px);
  display: grid;
  gap: 1rem;
  padding: 1.3rem;
  border-radius: 28px;
  border: 1px solid rgba(20, 33, 61, 0.12);
  background: rgba(255, 252, 248, 0.96);
  box-shadow: 0 36px 80px rgba(20, 33, 61, 0.22);
}

.admin-problem-delete-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-delete-summary {
  padding: 0.9rem 1rem;
  border-radius: 16px;
  border: 1px solid rgba(185, 28, 28, 0.16);
  background: rgba(255, 243, 243, 0.82);
  color: var(--danger);
}

.admin-problem-confirm-copy {
  margin: 0.4rem 0 0;
  color: var(--ink-soft);
}

.compact-state {
  min-height: 8rem;
}

@media (max-width: 1100px) {
  .admin-problems-toolbar,
  .admin-problem-editor-header {
    flex-direction: column;
  }

  .admin-problems-actions {
    justify-content: flex-start;
  }

  .admin-problems-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-problem-list {
    max-height: 24rem;
  }
}

@media (max-width: 720px) {
  .admin-problem-form-grid,
  .admin-problem-search,
  .admin-problem-create-row {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-problem-search,
  .admin-problem-create-row {
    display: grid;
  }

  .admin-problem-item-head,
  .admin-problem-item-limits {
    align-items: start;
    flex-direction: column;
  }

  .admin-problem-section-actions,
  .dialog-actions {
    justify-content: stretch;
  }
}
</style>
