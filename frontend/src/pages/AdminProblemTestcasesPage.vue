<template>
  <section class="page-grid single-column">
    <article class="panel admin-testcases-shell">
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
          <AdminTestcasesSidebar
            v-model:title-search-input="titleSearchInput"
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

          <AdminTestcasesEditor
            :editor-state="testcaseEditorState"
            :editor-draft="testcaseEditorDraft"
            :editor-actions="testcaseEditorActions"
          />
        </div>
      </template>
    </article>
  </section>
</template>

<script setup>
import { computed } from 'vue'

import AdminTestcasesEditor from '@/components/admin-testcases/AdminTestcasesEditor.vue'
import AdminTestcasesSidebar from '@/components/admin-testcases/AdminTestcasesSidebar.vue'
import AdminTestcasesToolbar from '@/components/admin-testcases/AdminTestcasesToolbar.vue'
import { useAdminProblemTestcasesPage } from '@/composables/useAdminProblemTestcasesPage'

const {
  authState,
  isAuthenticated,
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

<style scoped>
.admin-testcases-shell {
  display: grid;
  gap: 1.25rem;
  --admin-testcases-shell-surface: linear-gradient(
    180deg,
    rgba(246, 248, 251, 0.98),
    rgba(239, 243, 248, 0.94)
  );
  --admin-testcases-shell-border: rgba(148, 163, 184, 0.18);
  --admin-testcases-section-surface: linear-gradient(
    180deg,
    rgba(255, 255, 255, 0.98),
    rgba(248, 250, 252, 0.95)
  );
  --admin-testcases-section-border: rgba(148, 163, 184, 0.12);
  --admin-testcases-nested-surface: rgba(255, 255, 255, 0.98);
  --admin-testcases-nested-border: rgba(148, 163, 184, 0.14);
  --admin-testcases-shell-shadow:
    0 18px 36px rgba(20, 33, 61, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.7);
  --admin-testcases-section-shadow:
    0 12px 28px rgba(20, 33, 61, 0.04),
    inset 0 1px 0 rgba(255, 255, 255, 0.76);
}

.admin-testcases-layout {
  display: grid;
  grid-template-columns: minmax(320px, 380px) minmax(0, 1fr);
  gap: 1rem;
  align-items: start;
}

@media (max-width: 1100px) {
  .admin-testcases-layout {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
