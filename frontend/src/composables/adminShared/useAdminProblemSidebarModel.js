import { computed, unref } from 'vue'

function unwrapRecordValues(record){
  const nextRecord = {}

  Object.entries(record ?? {}).forEach(([key, value]) => {
    nextRecord[key] = unref(value)
  })

  return nextRecord
}

export function useAdminProblemSidebarModel({
  workspace,
  busySection,
  formatCount,
  formatProblemLimit,
  titleSearchInputId,
  problemIdSearchInputId,
  create
}){
  return computed(() => ({
    model: {
      searchMode: workspace.query.searchMode.value,
      titleSearchInput: workspace.query.titleSearchInput.value,
      problemIdSearchInput: workspace.query.problemIdSearchInput.value,
      titleSearchInputId,
      problemIdSearchInputId,
      isLoadingProblems: workspace.isLoadingProblems.value,
      busySection: unref(busySection),
      hasAppliedSearch: workspace.query.hasAppliedSearch.value,
      problemListCaption: workspace.query.problemListCaption.value,
      problemCount: workspace.problemCatalogResource.problemCount.value,
      listErrorMessage: workspace.problemCatalogResource.listErrorMessage.value,
      emptyProblemListMessage: workspace.query.emptyProblemListMessage.value,
      problems: workspace.problemCatalogResource.problems.value,
      selectedProblemId: workspace.selectedProblemId.value,
      formatCount,
      formatProblemLimit
    },
    actions: {
      updateTitleSearchInput: workspace.query.updateTitleSearchInput,
      updateProblemIdSearchInput: workspace.query.updateProblemIdSearchInput,
      setSearchMode: workspace.query.setSearchMode,
      submitSearch: workspace.query.submitSearch,
      resetSearch: workspace.query.resetSearch,
      selectProblem: workspace.selectProblem
    },
    create: create
      ? {
        model: unwrapRecordValues(create.model),
        actions: create.actions
      }
      : undefined
  }))
}
