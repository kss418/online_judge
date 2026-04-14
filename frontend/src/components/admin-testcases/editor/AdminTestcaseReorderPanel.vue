<template>
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

    <div class="admin-testcases-summary-layout">
      <div class="admin-testcases-summary-panel">
        <div v-if="isLoadingTestcases" class="empty-state compact-state">
          <p>테스트케이스를 불러오는 중입니다.</p>
        </div>

        <div v-else-if="testcaseErrorMessage" class="empty-state error-state compact-state">
          <p>{{ testcaseErrorMessage }}</p>
        </div>

        <div v-else-if="!testcaseItems.length" class="empty-state compact-state">
          <p>등록된 테스트케이스가 아직 없습니다.</p>
        </div>

        <div v-else class="admin-testcases-summary-list">
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
              입력 {{ describeTestcaseContent(testcase.input_char_count, testcase.input_line_count) }}
            </span>
            <span class="admin-testcase-summary-copy">
              출력 {{ describeTestcaseContent(testcase.output_char_count, testcase.output_line_count) }}
            </span>
          </button>
        </div>
      </div>

      <slot />
    </div>
  </article>
</template>

<script setup>
import { ref } from 'vue'

const props = defineProps({
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
  canMoveTestcases: {
    type: Boolean,
    required: true
  },
  isMovingTestcase: {
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
  'update:viewTestcaseOrderInput',
  'view-selected-testcase',
  'select-testcase',
  'move-testcase'
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
.admin-testcases-section {
  display: grid;
  gap: 1rem;
  padding: 1.2rem;
  border: 1px solid var(--admin-testcases-section-border);
  border-radius: 20px;
  background: var(--admin-testcases-section-surface);
  box-shadow: var(--admin-testcases-section-shadow);
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

@media (max-width: 720px) {
  .admin-testcases-order-form {
    display: grid;
    align-items: stretch;
  }

  .admin-testcases-summary-layout {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
