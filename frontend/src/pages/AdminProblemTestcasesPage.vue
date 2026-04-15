<template>
  <AdminSplitWorkspaceShell
    :access-state="accessState"
    :access-message="accessMessage"
  >
    <template #toolbar>
      <AdminTestcasesToolbar
        :toolbar-status-label="toolbarStatusLabel"
        :toolbar-status-tone="toolbarStatusTone"
        :can-manage-problems="canManageProblems"
        :busy-section="busySection"
        :is-loading-problems="isLoadingProblems"
        :is-loading-problem="isLoadingProblem"
        :is-loading-testcases="isLoadingTestcases"
        :selected-problem-id="selectedProblemId"
        @refresh="refreshPage"
      />
    </template>

    <template #sidebar>
      <AdminProblemSelectionSidebar
        v-model:title-search-input="titleSearchInput"
        title-search-input-id="admin-testcase-problem-search"
        problem-id-search-input-id="admin-testcase-problem-id-search"
        :search-mode="searchMode"
        :problem-id-search-input="problemIdSearchInput"
        :is-loading-problems="isLoadingProblems"
        :busy-section="busySection"
        :has-applied-search="hasAppliedSearch"
        :problem-list-caption="problemListCaption"
        :problem-count="problemCount"
        :list-error-message="listErrorMessage"
        :empty-problem-list-message="emptyProblemListMessage"
        :problems="problems"
        :selected-problem-id="selectedProblemId"
        :format-count="formatCount"
        :format-problem-limit="formatProblemLimit"
        @set-search-mode="setSearchMode"
        @problem-id-input="handleProblemIdSearchInput"
        @submit-search="submitSearch"
        @reset-search="resetSearch"
        @select-problem="selectProblem"
      />
    </template>

    <AdminTestcasesEditor
      :editor-state="testcaseEditorState"
      :editor-draft="testcaseEditorDraft"
      :editor-actions="testcaseEditorActions"
    />
  </AdminSplitWorkspaceShell>
</template>

<script setup>
import { computed } from 'vue'

import AdminProblemSelectionSidebar from '@/components/adminShared/AdminProblemSelectionSidebar.vue'
import AdminSplitWorkspaceShell from '@/components/adminShared/AdminSplitWorkspaceShell.vue'
import AdminTestcasesEditor from '@/components/admin-testcases/AdminTestcasesEditor.vue'
import AdminTestcasesToolbar from '@/components/admin-testcases/AdminTestcasesToolbar.vue'
import { useAdminProblemTestcasesPage } from '@/composables/useAdminProblemTestcasesPage'

const {
  accessState,
  accessMessage,
  canManageProblems,
  selectedProblemId,
  isLoadingProblems,
  isLoadingProblem,
  isLoadingTestcases,
  isLoadingSelectedTestcase,
  listErrorMessage,
  problemErrorMessage,
  testcaseErrorMessage,
  selectedTestcaseErrorMessage,
  searchMode,
  titleSearchInput,
  problemIdSearchInput,
  problems,
  problemDetail,
  testcaseItems,
  newTestcaseInput,
  newTestcaseOutput,
  testcaseZipInputKey,
  selectedTestcaseOrder,
  selectedTestcaseInputDraft,
  selectedTestcaseOutputDraft,
  viewTestcaseOrderInput,
  busySection,
  problemCount,
  toolbarStatusLabel,
  toolbarStatusTone,
  hasAppliedSearch,
  problemListCaption,
  emptyProblemListMessage,
  isCreatingTestcase,
  isUploadingTestcaseZip,
  isDeletingSelectedTestcase,
  isMovingTestcase,
  isSavingSelectedTestcase,
  selectedTestcaseZipName,
  selectedTestcase,
  canCreateTestcase,
  canUploadTestcaseZip,
  canDeleteSelectedTestcase,
  canMoveTestcases,
  canSaveSelectedTestcase,
  canViewSpecificTestcase,
  formatCount,
  formatProblemLimit,
  describeTestcaseContent,
  isLastTestcase,
  setSearchMode,
  handleProblemIdSearchInput,
  submitSearch,
  resetSearch,
  selectProblem,
  refreshPage,
  handleTestcaseZipFileChange,
  handleUploadTestcaseZip,
  handleCreateTestcase,
  selectTestcase,
  handleDeleteSelectedTestcase,
  handleMoveTestcase,
  handleSaveSelectedTestcase,
  handleViewSelectedTestcase,
  setTestcaseSummaryElement
} = useAdminProblemTestcasesPage()

const testcaseEditorState = computed(() => ({
  selectedProblemId: selectedProblemId.value,
  isLoadingProblem: isLoadingProblem.value,
  problemErrorMessage: problemErrorMessage.value,
  problemDetail: problemDetail.value,
  busySection: busySection.value,
  canUploadTestcaseZip: canUploadTestcaseZip.value,
  isUploadingTestcaseZip: isUploadingTestcaseZip.value,
  canCreateTestcase: canCreateTestcase.value,
  isCreatingTestcase: isCreatingTestcase.value,
  canViewSpecificTestcase: canViewSpecificTestcase.value,
  isLoadingTestcases: isLoadingTestcases.value,
  isLoadingSelectedTestcase: isLoadingSelectedTestcase.value,
  testcaseItems: testcaseItems.value,
  testcaseErrorMessage: testcaseErrorMessage.value,
  selectedTestcaseErrorMessage: selectedTestcaseErrorMessage.value,
  selectedTestcaseOrder: selectedTestcaseOrder.value,
  selectedTestcase: selectedTestcase.value,
  canDeleteSelectedTestcase: canDeleteSelectedTestcase.value,
  isDeletingSelectedTestcase: isDeletingSelectedTestcase.value,
  canMoveTestcases: canMoveTestcases.value,
  isMovingTestcase: isMovingTestcase.value,
  canSaveSelectedTestcase: canSaveSelectedTestcase.value,
  isSavingSelectedTestcase: isSavingSelectedTestcase.value,
  formatCount,
  describeTestcaseContent,
  isLastTestcase,
  setTestcaseSummaryElement
}))

const testcaseEditorDraft = computed(() => ({
  testcaseZipInputKey: testcaseZipInputKey.value,
  selectedTestcaseZipName: selectedTestcaseZipName.value,
  newTestcaseInput: newTestcaseInput.value,
  newTestcaseOutput: newTestcaseOutput.value,
  viewTestcaseOrderInput: viewTestcaseOrderInput.value,
  selectedTestcaseInputDraft: selectedTestcaseInputDraft.value,
  selectedTestcaseOutputDraft: selectedTestcaseOutputDraft.value
}))

function updateNewTestcaseInput(value){
  newTestcaseInput.value = value
}

function updateNewTestcaseOutput(value){
  newTestcaseOutput.value = value
}

function updateViewTestcaseOrderInput(value){
  viewTestcaseOrderInput.value = value
}

function updateSelectedTestcaseInputDraft(value){
  selectedTestcaseInputDraft.value = value
}

function updateSelectedTestcaseOutputDraft(value){
  selectedTestcaseOutputDraft.value = value
}

const testcaseEditorActions = {
  updateNewTestcaseInput,
  updateNewTestcaseOutput,
  updateViewTestcaseOrderInput,
  updateSelectedTestcaseInputDraft,
  updateSelectedTestcaseOutputDraft,
  changeTestcaseZip: handleTestcaseZipFileChange,
  uploadTestcaseZip: handleUploadTestcaseZip,
  createTestcase: handleCreateTestcase,
  viewSelectedTestcase: handleViewSelectedTestcase,
  selectTestcase,
  deleteSelectedTestcase: handleDeleteSelectedTestcase,
  moveTestcase: handleMoveTestcase,
  saveSelectedTestcase: handleSaveSelectedTestcase
}
</script>
