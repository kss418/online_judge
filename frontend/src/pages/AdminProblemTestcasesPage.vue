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

    <AdminTestcasesEditor :editor="testcaseEditor" />
  </AdminSplitWorkspaceShell>
</template>

<script setup>
import AdminProblemSelectionSidebar from '@/components/adminShared/AdminProblemSelectionSidebar.vue'
import AdminSplitWorkspaceShell from '@/components/adminShared/AdminSplitWorkspaceShell.vue'
import AdminTestcasesEditor from '@/components/admin-testcases/AdminTestcasesEditor.vue'
import AdminTestcasesToolbar from '@/components/admin-testcases/AdminTestcasesToolbar.vue'
import { useTestcaseEditorSections } from '@/composables/adminProblemTestcases/useTestcaseEditorSections'
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

const testcaseEditor = useTestcaseEditorSections({
  selectedProblemId,
  isLoadingProblem,
  problemErrorMessage,
  problemDetail,
  busySection,
  canUploadTestcaseZip,
  isUploadingTestcaseZip,
  canCreateTestcase,
  isCreatingTestcase,
  canViewSpecificTestcase,
  isLoadingTestcases,
  isLoadingSelectedTestcase,
  testcaseItems,
  testcaseErrorMessage,
  selectedTestcaseErrorMessage,
  selectedTestcaseOrder,
  selectedTestcase,
  canDeleteSelectedTestcase,
  isDeletingSelectedTestcase,
  canMoveTestcases,
  isMovingTestcase,
  canSaveSelectedTestcase,
  isSavingSelectedTestcase,
  formatCount,
  describeTestcaseContent,
  isLastTestcase,
  setTestcaseSummaryElement,
  testcaseZipInputKey,
  selectedTestcaseZipName,
  newTestcaseInput,
  newTestcaseOutput,
  viewTestcaseOrderInput,
  selectedTestcaseInputDraft,
  selectedTestcaseOutputDraft,
  handleTestcaseZipFileChange,
  handleUploadTestcaseZip,
  handleCreateTestcase,
  selectTestcase,
  handleDeleteSelectedTestcase,
  handleMoveTestcase,
  handleSaveSelectedTestcase,
  handleViewSelectedTestcase
})
</script>
