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
            v-model:new-testcase-input="newTestcaseInput"
            v-model:new-testcase-output="newTestcaseOutput"
            v-model:view-testcase-order-input="viewTestcaseOrderInput"
            v-model:selected-testcase-input-draft="selectedTestcaseInputDraft"
            v-model:selected-testcase-output-draft="selectedTestcaseOutputDraft"
            :selected-problem-id="selectedProblemId"
            :is-loading-problem="isLoadingProblem"
            :problem-error-message="problemErrorMessage"
            :problem-detail="problemDetail"
            :testcase-zip-input-key="testcaseZipInputKey"
            :busy-section="busySection"
            :selected-testcase-zip-name="selectedTestcaseZipName"
            :can-upload-testcase-zip="canUploadTestcaseZip"
            :is-uploading-testcase-zip="isUploadingTestcaseZip"
            :can-create-testcase="canCreateTestcase"
            :is-creating-testcase="isCreatingTestcase"
            :can-view-specific-testcase="canViewSpecificTestcase"
            :is-loading-testcases="isLoadingTestcases"
            :testcase-items="testcaseItems"
            :testcase-error-message="testcaseErrorMessage"
            :selected-testcase-order="selectedTestcaseOrder"
            :selected-testcase="selectedTestcase"
            :can-delete-last-testcase="canDeleteLastTestcase"
            :is-deleting-last-testcase="isDeletingLastTestcase"
            :can-save-selected-testcase="canSaveSelectedTestcase"
            :is-saving-selected-testcase="isSavingSelectedTestcase"
            :format-count="formatCount"
            :describe-testcase-content="describeTestcaseContent"
            :is-last-testcase="isLastTestcase"
            :set-testcase-summary-element="setTestcaseSummaryElement"
            @testcase-zip-change="handleTestcaseZipFileChange"
            @upload-testcase-zip="handleUploadTestcaseZip"
            @create-testcase="handleCreateTestcase"
            @view-selected-testcase="handleViewSelectedTestcase"
            @select-testcase="selectTestcase"
            @delete-last-testcase="handleDeleteLastTestcase"
            @save-selected-testcase="handleSaveSelectedTestcase"
          />
        </div>
      </template>
    </article>
  </section>
</template>

<script setup>
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
  listErrorMessage,
  problemErrorMessage,
  testcaseErrorMessage,
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
  isDeletingLastTestcase,
  isSavingSelectedTestcase,
  selectedTestcaseZipName,
  selectedTestcase,
  canCreateTestcase,
  canUploadTestcaseZip,
  canDeleteLastTestcase,
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
  handleDeleteLastTestcase,
  handleSaveSelectedTestcase,
  handleViewSelectedTestcase,
  setTestcaseSummaryElement
} = useAdminProblemTestcasesPage()
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
