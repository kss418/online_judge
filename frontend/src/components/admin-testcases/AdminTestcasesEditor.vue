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
      <AdminTestcasesEditorHeader
        :problem-detail="problemDetail"
        :testcase-items="testcaseItems"
        :format-count="formatCount"
      />

      <AdminTestcaseUploadControls
        :testcase-zip-input-key="testcaseZipInputKey"
        :busy-section="busySection"
        :selected-testcase-zip-name="selectedTestcaseZipName"
        :can-upload-testcase-zip="canUploadTestcaseZip"
        :is-uploading-testcase-zip="isUploadingTestcaseZip"
        @testcase-zip-change="$emit('testcase-zip-change', $event)"
        @upload-testcase-zip="$emit('upload-testcase-zip')"
      />

      <AdminTestcaseCreateSection
        :new-testcase-input="newTestcaseInput"
        :new-testcase-output="newTestcaseOutput"
        :busy-section="busySection"
        :can-create-testcase="canCreateTestcase"
        :is-creating-testcase="isCreatingTestcase"
        @update:new-testcase-input="$emit('update:newTestcaseInput', $event)"
        @update:new-testcase-output="$emit('update:newTestcaseOutput', $event)"
        @create-testcase="$emit('create-testcase')"
      />

      <AdminTestcaseReorderPanel
        :view-testcase-order-input="viewTestcaseOrderInput"
        :can-view-specific-testcase="canViewSpecificTestcase"
        :is-loading-testcases="isLoadingTestcases"
        :testcase-items="testcaseItems"
        :testcase-error-message="testcaseErrorMessage"
        :selected-testcase-order="selectedTestcaseOrder"
        :selected-testcase="selectedTestcase"
        :can-move-testcases="canMoveTestcases"
        :is-moving-testcase="isMovingTestcase"
        :format-count="formatCount"
        :describe-testcase-content="describeTestcaseContent"
        :is-last-testcase="isLastTestcase"
        :set-testcase-summary-element="setTestcaseSummaryElement"
        @update:view-testcase-order-input="$emit('update:viewTestcaseOrderInput', $event)"
        @view-selected-testcase="$emit('view-selected-testcase')"
        @select-testcase="$emit('select-testcase', $event)"
        @move-testcase="$emit('move-testcase', $event)"
      >
        <AdminTestcaseDetailCard
          :is-loading-testcases="isLoadingTestcases"
          :testcase-error-message="testcaseErrorMessage"
          :testcase-items="testcaseItems"
          :is-loading-selected-testcase="isLoadingSelectedTestcase"
          :selected-testcase-error-message="selectedTestcaseErrorMessage"
          :selected-testcase="selectedTestcase"
          :can-delete-selected-testcase="canDeleteSelectedTestcase"
          :is-deleting-selected-testcase="isDeletingSelectedTestcase"
          :busy-section="busySection"
          :selected-testcase-input-draft="selectedTestcaseInputDraft"
          :selected-testcase-output-draft="selectedTestcaseOutputDraft"
          :can-save-selected-testcase="canSaveSelectedTestcase"
          :is-saving-selected-testcase="isSavingSelectedTestcase"
          :format-count="formatCount"
          @delete-selected-testcase="$emit('delete-selected-testcase')"
          @update:selected-testcase-input-draft="$emit('update:selectedTestcaseInputDraft', $event)"
          @update:selected-testcase-output-draft="$emit('update:selectedTestcaseOutputDraft', $event)"
          @save-selected-testcase="$emit('save-selected-testcase')"
        />
      </AdminTestcaseReorderPanel>
    </template>
  </section>
</template>

<script setup>
import AdminTestcaseCreateSection from '@/components/admin-testcases/editor/AdminTestcaseCreateSection.vue'
import AdminTestcaseDetailCard from '@/components/admin-testcases/editor/AdminTestcaseDetailCard.vue'
import AdminTestcaseReorderPanel from '@/components/admin-testcases/editor/AdminTestcaseReorderPanel.vue'
import AdminTestcasesEditorHeader from '@/components/admin-testcases/editor/AdminTestcasesEditorHeader.vue'
import AdminTestcaseUploadControls from '@/components/admin-testcases/editor/AdminTestcaseUploadControls.vue'

defineProps({
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
  isLoadingSelectedTestcase: {
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
  selectedTestcaseErrorMessage: {
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

defineEmits([
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
</style>
