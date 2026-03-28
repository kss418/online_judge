<template>
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
          <h3 class="admin-testcases-problem-heading">
            <span class="admin-testcases-problem-heading-id-group">
              <span class="admin-testcases-problem-heading-id">
                #{{ formatCount(problemDetail.problem_id) }}
              </span>
              <span class="admin-testcases-problem-heading-divider" aria-hidden="true">·</span>
            </span>
            <span class="admin-testcases-problem-heading-text">{{ problemDetail.title }}</span>
          </h3>
          <p class="admin-testcases-editor-copy">
            요약 리스트에서 특정 테스트케이스를 고르고, 선택한 항목만 편집해서 저장할 수 있습니다.
          </p>
          <p
            v-if="testcaseItems.length > 1"
            class="admin-testcases-editor-copy"
          >
            요약 카드들을 드래그해서 순서를 바로 바꿀 수 있습니다.
          </p>
        </div>
        <div class="admin-testcases-editor-side">
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

      <article class="admin-testcases-section admin-testcases-section--upload">
        <div class="panel-header">
          <div>
            <p class="panel-kicker">upload</p>
            <h3>ZIP 업로드</h3>
            <p class="admin-testcases-section-copy">
              ZIP 업로드 시 기존 테스트케이스는 모두 교체됩니다. `001.in` / `001.out`부터
              `999.in` / `999.out`까지 연속 순번만 허용하며, 빈 파일은 허용됩니다.
            </p>
          </div>
        </div>

        <div class="admin-testcase-upload-panel">
          <label class="field-block">
            <span class="field-label">ZIP 파일</span>
            <input
              :key="testcaseZipInputKey"
              class="admin-testcase-file-input"
              type="file"
              accept=".zip,application/zip"
              :disabled="Boolean(busySection)"
              @change="$emit('testcase-zip-change', $event)"
            />
          </label>

          <p v-if="selectedTestcaseZipName" class="admin-testcase-selected-file">
            선택한 파일: {{ selectedTestcaseZipName }}
          </p>
        </div>

        <div class="admin-testcases-actions">
          <button
            type="button"
            class="primary-button"
            :disabled="!canUploadTestcaseZip"
            @click="$emit('upload-testcase-zip')"
          >
            {{ isUploadingTestcaseZip ? '업로드 중...' : 'ZIP 업로드' }}
          </button>
        </div>
      </article>

      <article class="admin-testcases-section admin-testcases-section--append">
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
              :value="newTestcaseInput"
              class="admin-testcases-textarea"
              spellcheck="false"
              :disabled="Boolean(busySection)"
              placeholder="빈 입력도 허용됩니다."
              @input="$emit('update:newTestcaseInput', $event.target.value)"
            />
          </label>

          <label class="field-block">
            <span class="field-label">출력</span>
            <textarea
              :value="newTestcaseOutput"
              class="admin-testcases-textarea"
              spellcheck="false"
              :disabled="Boolean(busySection)"
              placeholder="빈 출력도 허용됩니다."
              @input="$emit('update:newTestcaseOutput', $event.target.value)"
            />
          </label>
        </div>

        <div class="admin-testcases-actions">
          <button
            type="button"
            class="primary-button"
            :disabled="!canCreateTestcase"
            @click="$emit('create-testcase')"
          >
            {{ isCreatingTestcase ? '추가 중...' : '테스트케이스 추가' }}
          </button>
        </div>
      </article>

      <article class="admin-testcases-section admin-testcases-section--list">
        <div class="panel-header">
          <div>
            <p class="panel-kicker">list</p>
            <h3>테스트케이스 요약</h3>
          </div>
        </div>

        <form class="admin-testcases-order-form" @submit.prevent="$emit('view-selected-testcase')">
          <label class="field-block admin-testcases-order-field">
            <span class="field-label">특정 순번 보기</span>
            <input
              :value="viewTestcaseOrderInput"
              class="field-input"
              type="text"
              inputmode="numeric"
              :disabled="isLoadingTestcases || !testcaseItems.length"
              placeholder="예: 37"
              @input="$emit('update:viewTestcaseOrderInput', $event.target.value)"
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
              @click="$emit('select-testcase', testcaseItems[0].testcase_order)"
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
                :class="{
                  'is-active': testcase.testcase_order === selectedTestcaseOrder,
                  'is-draggable': canMoveTestcases,
                  'is-dragging': testcase.testcase_order === draggingTestcaseOrder,
                  'is-drop-target': testcase.testcase_order === dropTargetTestcaseOrder
                }"
                :ref="(element) => setTestcaseSummaryElement(testcase.testcase_order, element)"
                :draggable="canMoveTestcases"
                @click="$emit('select-testcase', testcase.testcase_order)"
                @dragstart="handleSummaryDragStart($event, testcase.testcase_order)"
                @dragover="handleSummaryDragOver($event, testcase.testcase_order)"
                @drop="handleSummaryDrop($event, testcase.testcase_order)"
                @dragend="handleSummaryDragEnd"
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
                type="button"
                class="ghost-button admin-testcase-delete-button"
                :disabled="!canDeleteSelectedTestcase"
                @click="$emit('delete-selected-testcase')"
              >
                {{ isDeletingSelectedTestcase ? '삭제 중...' : '테스트케이스 삭제' }}
              </button>
            </div>

            <div class="admin-testcases-grid">
              <label class="field-block">
                <span class="field-label">입력</span>
                <textarea
                  :value="selectedTestcaseInputDraft"
                  class="admin-testcases-textarea"
                  spellcheck="false"
                  :disabled="Boolean(busySection)"
                  placeholder="빈 입력도 허용됩니다."
                  @input="$emit('update:selectedTestcaseInputDraft', $event.target.value)"
                />
              </label>

              <label class="field-block">
                <span class="field-label">출력</span>
                <textarea
                  :value="selectedTestcaseOutputDraft"
                  class="admin-testcases-textarea"
                  spellcheck="false"
                  :disabled="Boolean(busySection)"
                  placeholder="빈 출력도 허용됩니다."
                  @input="$emit('update:selectedTestcaseOutputDraft', $event.target.value)"
                />
              </label>
            </div>

            <div class="admin-testcases-actions">
              <button
                type="button"
                class="primary-button"
                :disabled="!canSaveSelectedTestcase"
                @click="$emit('save-selected-testcase')"
              >
                {{ isSavingSelectedTestcase ? '저장 중...' : '저장' }}
              </button>
            </div>
          </article>
        </div>
      </article>
    </template>
  </section>
</template>

<script setup>
import { ref } from 'vue'

const props = defineProps({
  selectedProblemId: {
    type: Number,
    required: true
  },
  isLoadingProblem: {
    type: Boolean,
    required: true
  },
  problemErrorMessage: {
    type: String,
    required: true
  },
  problemDetail: {
    type: Object,
    default: null
  },
  testcaseZipInputKey: {
    type: Number,
    required: true
  },
  busySection: {
    type: String,
    default: ''
  },
  selectedTestcaseZipName: {
    type: String,
    default: ''
  },
  canUploadTestcaseZip: {
    type: Boolean,
    required: true
  },
  isUploadingTestcaseZip: {
    type: Boolean,
    required: true
  },
  newTestcaseInput: {
    type: String,
    default: ''
  },
  newTestcaseOutput: {
    type: String,
    default: ''
  },
  canCreateTestcase: {
    type: Boolean,
    required: true
  },
  isCreatingTestcase: {
    type: Boolean,
    required: true
  },
  viewTestcaseOrderInput: {
    type: String,
    default: ''
  },
  canViewSpecificTestcase: {
    type: Boolean,
    required: true
  },
  isLoadingTestcases: {
    type: Boolean,
    required: true
  },
  testcaseItems: {
    type: Array,
    required: true
  },
  testcaseErrorMessage: {
    type: String,
    required: true
  },
  selectedTestcaseOrder: {
    type: Number,
    required: true
  },
  selectedTestcase: {
    type: Object,
    default: null
  },
  canDeleteSelectedTestcase: {
    type: Boolean,
    required: true
  },
  isDeletingSelectedTestcase: {
    type: Boolean,
    required: true
  },
  canMoveTestcases: {
    type: Boolean,
    required: true
  },
  isMovingTestcase: {
    type: Boolean,
    required: true
  },
  selectedTestcaseInputDraft: {
    type: String,
    default: ''
  },
  selectedTestcaseOutputDraft: {
    type: String,
    default: ''
  },
  canSaveSelectedTestcase: {
    type: Boolean,
    required: true
  },
  isSavingSelectedTestcase: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  },
  describeTestcaseContent: {
    type: Function,
    required: true
  },
  isLastTestcase: {
    type: Function,
    required: true
  },
  setTestcaseSummaryElement: {
    type: Function,
    required: true
  }
})

const emit = defineEmits([
  'testcase-zip-change',
  'upload-testcase-zip',
  'update:newTestcaseInput',
  'update:newTestcaseOutput',
  'create-testcase',
  'update:viewTestcaseOrderInput',
  'view-selected-testcase',
  'select-testcase',
  'delete-selected-testcase',
  'move-testcase',
  'update:selectedTestcaseInputDraft',
  'update:selectedTestcaseOutputDraft',
  'save-selected-testcase'
])

const draggingTestcaseOrder = ref(0)
const dropTargetTestcaseOrder = ref(0)

function resetDragState(){
  draggingTestcaseOrder.value = 0
  dropTargetTestcaseOrder.value = 0
}

function handleSummaryDragStart(event, testcaseOrder){
  if (!props.canMoveTestcases || props.isMovingTestcase) {
    event.preventDefault()
    return
  }

  draggingTestcaseOrder.value = testcaseOrder
  dropTargetTestcaseOrder.value = 0

  if (event.dataTransfer) {
    event.dataTransfer.effectAllowed = 'move'
    event.dataTransfer.setData('text/plain', String(testcaseOrder))
  }
}

function handleSummaryDragOver(event, testcaseOrder){
  if (
    !props.canMoveTestcases ||
    props.isMovingTestcase ||
    !draggingTestcaseOrder.value ||
    draggingTestcaseOrder.value === testcaseOrder
  ) {
    return
  }

  event.preventDefault()
  dropTargetTestcaseOrder.value = testcaseOrder

  if (event.dataTransfer) {
    event.dataTransfer.dropEffect = 'move'
  }
}

function handleSummaryDrop(event, testcaseOrder){
  if (!props.canMoveTestcases || props.isMovingTestcase) {
    return
  }

  event.preventDefault()
  const draggedOrderFromTransfer = Number.parseInt(
    event.dataTransfer?.getData('text/plain') ?? '',
    10
  )
  const sourceTestcaseOrder =
    Number.isInteger(draggedOrderFromTransfer) && draggedOrderFromTransfer > 0
      ? draggedOrderFromTransfer
      : draggingTestcaseOrder.value

  if (
    sourceTestcaseOrder > 0 &&
    testcaseOrder > 0 &&
    sourceTestcaseOrder !== testcaseOrder
  ) {
    emit('move-testcase', {
      sourceTestcaseOrder,
      targetTestcaseOrder: testcaseOrder
    })
  }

  resetDragState()
}

function handleSummaryDragEnd(){
  resetDragState()
}
</script>

<style scoped>
.admin-testcases-editor-panel {
  display: grid;
  gap: 1rem;
  min-height: 40rem;
  padding: 1rem;
  border: 1px solid var(--admin-testcases-shell-border);
  border-radius: 24px;
  background: var(--admin-testcases-shell-surface);
  box-shadow: var(--admin-testcases-shell-shadow);
}

.admin-testcases-editor-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-testcases-editor-copy,
.admin-testcases-section-copy {
  margin: 0;
  color: var(--ink-soft);
}

.admin-testcases-editor-copy {
  margin-top: 0.55rem;
}

.admin-testcases-section-copy {
  margin-top: 0.45rem;
}

.admin-testcases-problem-heading {
  display: flex;
  align-items: baseline;
  gap: 0.75rem;
  flex-wrap: wrap;
  min-width: 0;
}

.admin-testcases-problem-heading-id-group {
  display: inline-flex;
  align-items: baseline;
  gap: 0.5rem;
  white-space: nowrap;
  color: var(--ink-soft);
  flex-shrink: 0;
}

.admin-testcases-problem-heading-id {
  font-weight: 800;
  letter-spacing: -0.03em;
}

.admin-testcases-problem-heading-divider {
  color: rgba(20, 33, 61, 0.34);
  font-weight: 700;
}

.admin-testcases-problem-heading-text {
  min-width: 0;
  color: var(--ink-strong);
}

.admin-testcases-editor-side {
  display: grid;
  gap: 0.7rem;
  justify-items: end;
  flex: 0 0 auto;
}

.admin-testcases-editor-actions {
  display: flex;
  gap: 0.75rem;
  flex-wrap: wrap;
  justify-content: flex-end;
}

.admin-testcases-section,
.admin-testcase-card {
  border: 1px solid var(--admin-testcases-section-border);
  border-radius: 20px;
  box-shadow: var(--admin-testcases-section-shadow);
}

.admin-testcases-section {
  padding: 1.2rem;
  background: var(--admin-testcases-section-surface);
}

.admin-testcase-card {
  min-width: 0;
  overflow: hidden;
  padding: 1rem;
  border-color: var(--admin-testcases-nested-border);
  background: var(--admin-testcases-nested-surface);
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
  background: var(--admin-testcases-nested-surface);
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

.admin-testcase-upload-panel {
  display: grid;
  gap: 0.75rem;
  padding: 0.95rem 1rem;
  border: 1px dashed rgba(148, 163, 184, 0.24);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.72);
}

.admin-testcase-file-input {
  display: block;
  width: 100%;
  padding: 0.85rem 1rem;
  border-radius: 18px;
  border: 1px dashed rgba(20, 33, 61, 0.18);
  background: var(--admin-testcases-nested-surface);
  color: var(--ink-strong);
  font: inherit;
}

.admin-testcase-selected-file {
  margin: 0;
  color: var(--ink-soft);
  word-break: break-all;
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
  min-width: 0;
  padding: 0.85rem;
  border: 1px solid var(--admin-testcases-section-border);
  border-radius: 20px;
  background: var(--admin-testcases-section-surface);
  box-shadow: var(--admin-testcases-section-shadow);
  position: relative;
  z-index: 1;
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
  border: 1px solid var(--admin-testcases-nested-border);
  border-radius: 18px;
  background: var(--admin-testcases-nested-surface);
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

.admin-testcase-summary-item.is-draggable {
  cursor: grab;
}

.admin-testcase-summary-item.is-draggable:active {
  cursor: grabbing;
}

.admin-testcase-summary-item.is-dragging {
  opacity: 0.55;
  transform: scale(0.985);
}

.admin-testcase-summary-item:hover,
.admin-testcase-summary-item.is-active {
  border-color: rgba(217, 119, 6, 0.34);
  background: rgba(255, 247, 237, 0.96);
  box-shadow: 0 10px 24px rgba(217, 119, 6, 0.1);
}

.admin-testcase-summary-item.is-drop-target,
.admin-testcase-summary-item.is-drop-target:hover,
.admin-testcase-summary-item.is-drop-target.is-active {
  border-color: rgba(14, 116, 144, 0.42);
  background: rgba(236, 254, 255, 0.96);
  box-shadow: 0 12px 28px rgba(14, 116, 144, 0.14);
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

@media (max-width: 720px) {
  .admin-testcases-grid {
    grid-template-columns: 1fr;
  }

  .admin-testcases-order-form {
    display: grid;
    align-items: stretch;
  }

  .admin-testcases-summary-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-testcase-card-header,
  .admin-testcases-editor-header {
    align-items: start;
    flex-direction: column;
  }

  .admin-testcases-editor-side {
    width: 100%;
    justify-items: start;
  }

  .admin-testcases-editor-actions {
    justify-content: flex-start;
  }

  .admin-testcases-actions {
    justify-content: stretch;
  }
}
</style>
