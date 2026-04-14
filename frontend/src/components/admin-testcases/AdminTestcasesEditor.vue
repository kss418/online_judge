<template>
  <section class="admin-testcases-editor-panel">
    <div v-if="!editorState.selectedProblemId" class="empty-state">
      <p>왼쪽 목록에서 문제를 선택하면 테스트케이스를 관리할 수 있습니다.</p>
    </div>

    <div v-else-if="editorState.isLoadingProblem" class="empty-state">
      <p>문제 정보를 불러오는 중입니다.</p>
    </div>

    <div v-else-if="editorState.problemErrorMessage" class="empty-state error-state">
      <p>{{ editorState.problemErrorMessage }}</p>
    </div>

    <template v-else-if="editorState.problemDetail">
      <AdminTestcasesEditorHeader :section="headerSection" />

      <AdminTestcaseUploadControls :section="uploadSection" />

      <AdminTestcaseCreateSection :section="createSection" />

      <AdminTestcaseReorderPanel :section="reorderSection">
        <AdminTestcaseDetailCard :section="detailSection" />
      </AdminTestcaseReorderPanel>
    </template>
  </section>
</template>

<script setup>
import { computed } from 'vue'

import AdminTestcaseCreateSection from '@/components/admin-testcases/editor/AdminTestcaseCreateSection.vue'
import AdminTestcaseDetailCard from '@/components/admin-testcases/editor/AdminTestcaseDetailCard.vue'
import AdminTestcaseReorderPanel from '@/components/admin-testcases/editor/AdminTestcaseReorderPanel.vue'
import AdminTestcasesEditorHeader from '@/components/admin-testcases/editor/AdminTestcasesEditorHeader.vue'
import AdminTestcaseUploadControls from '@/components/admin-testcases/editor/AdminTestcaseUploadControls.vue'

const props = defineProps({
  editorState: {
    type: Object,
    required: true
  },
  editorDraft: {
    type: Object,
    required: true
  },
  editorActions: {
    type: Object,
    required: true
  }
})

const headerSection = computed(() => ({
  model: {
    problemDetail: props.editorState.problemDetail,
    testcaseItems: props.editorState.testcaseItems,
    formatCount: props.editorState.formatCount
  },
  actions: {}
}))

const uploadSection = computed(() => ({
  model: {
    testcaseZipInputKey: props.editorDraft.testcaseZipInputKey,
    busySection: props.editorState.busySection,
    selectedTestcaseZipName: props.editorDraft.selectedTestcaseZipName,
    canUploadTestcaseZip: props.editorState.canUploadTestcaseZip,
    isUploadingTestcaseZip: props.editorState.isUploadingTestcaseZip
  },
  actions: {
    changeTestcaseZip: props.editorActions.changeTestcaseZip,
    uploadTestcaseZip: props.editorActions.uploadTestcaseZip
  }
}))

const createSection = computed(() => ({
  model: {
    newTestcaseInput: props.editorDraft.newTestcaseInput,
    newTestcaseOutput: props.editorDraft.newTestcaseOutput,
    busySection: props.editorState.busySection,
    canCreateTestcase: props.editorState.canCreateTestcase,
    isCreatingTestcase: props.editorState.isCreatingTestcase
  },
  actions: {
    updateNewTestcaseInput: props.editorActions.updateNewTestcaseInput,
    updateNewTestcaseOutput: props.editorActions.updateNewTestcaseOutput,
    createTestcase: props.editorActions.createTestcase
  }
}))

const reorderSection = computed(() => ({
  model: {
    viewTestcaseOrderInput: props.editorDraft.viewTestcaseOrderInput,
    canViewSpecificTestcase: props.editorState.canViewSpecificTestcase,
    isLoadingTestcases: props.editorState.isLoadingTestcases,
    testcaseItems: props.editorState.testcaseItems,
    testcaseErrorMessage: props.editorState.testcaseErrorMessage,
    selectedTestcaseOrder: props.editorState.selectedTestcaseOrder,
    selectedTestcase: props.editorState.selectedTestcase,
    canMoveTestcases: props.editorState.canMoveTestcases,
    isMovingTestcase: props.editorState.isMovingTestcase,
    formatCount: props.editorState.formatCount,
    describeTestcaseContent: props.editorState.describeTestcaseContent,
    isLastTestcase: props.editorState.isLastTestcase,
    setTestcaseSummaryElement: props.editorState.setTestcaseSummaryElement
  },
  actions: {
    updateViewTestcaseOrderInput: props.editorActions.updateViewTestcaseOrderInput,
    viewSelectedTestcase: props.editorActions.viewSelectedTestcase,
    selectTestcase: props.editorActions.selectTestcase,
    moveTestcase: props.editorActions.moveTestcase
  }
}))

const detailSection = computed(() => ({
  model: {
    isLoadingTestcases: props.editorState.isLoadingTestcases,
    testcaseErrorMessage: props.editorState.testcaseErrorMessage,
    testcaseItems: props.editorState.testcaseItems,
    isLoadingSelectedTestcase: props.editorState.isLoadingSelectedTestcase,
    selectedTestcaseErrorMessage: props.editorState.selectedTestcaseErrorMessage,
    selectedTestcase: props.editorState.selectedTestcase,
    canDeleteSelectedTestcase: props.editorState.canDeleteSelectedTestcase,
    isDeletingSelectedTestcase: props.editorState.isDeletingSelectedTestcase,
    busySection: props.editorState.busySection,
    selectedTestcaseInputDraft: props.editorDraft.selectedTestcaseInputDraft,
    selectedTestcaseOutputDraft: props.editorDraft.selectedTestcaseOutputDraft,
    canSaveSelectedTestcase: props.editorState.canSaveSelectedTestcase,
    isSavingSelectedTestcase: props.editorState.isSavingSelectedTestcase,
    formatCount: props.editorState.formatCount
  },
  actions: {
    deleteSelectedTestcase: props.editorActions.deleteSelectedTestcase,
    updateSelectedTestcaseInputDraft: props.editorActions.updateSelectedTestcaseInputDraft,
    updateSelectedTestcaseOutputDraft: props.editorActions.updateSelectedTestcaseOutputDraft,
    saveSelectedTestcase: props.editorActions.saveSelectedTestcase
  }
}))
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
