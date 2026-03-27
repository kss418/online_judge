<template>
  <section class="page-grid single-column">
    <article class="panel problems-panel">
      <ProblemsToolbar
        v-model:search-input="searchState.searchInput"
        :is-loading="isLoading"
        :error-message="errorMessage"
        :problem-count="problemCount"
        :visible-range-text="visibleRangeText"
        :has-applied-title-filter="hasAppliedTitleFilter"
        @submit-search="submitSearch"
        @reset-search="resetSearch"
      />

      <ProblemsFilters
        :show-problem-state-filters="showProblemStateFilters"
        :is-loading="isLoading"
        :problem-state-filter-options="problemStateFilterOptions"
        :problem-sort-options="problemSortOptions"
        :applied-state-filter="appliedStateFilter"
        :applied-sort-key="appliedSortKey"
        :applied-sort-direction="appliedSortDirection"
        @apply-state-filter="applyStateFilter"
        @cycle-sort="cycleSort"
      />

      <div v-if="isLoading" class="empty-state">
        <p>문제 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else-if="!problemCount" class="empty-state">
        <p>{{ emptyStateMessage }}</p>
      </div>

      <ProblemsTable
        v-else
        :problems="pagedProblems"
        :problem-sort-options="problemSortOptions"
        :applied-sort-key="appliedSortKey"
        :applied-sort-direction="appliedSortDirection"
        @cycle-sort="cycleSort"
      />

      <PaginationBar
        v-if="!isLoading && !errorMessage && problemCount > 50"
        v-model:jump-input="searchState.pageJumpInput"
        :current-page="currentPage"
        :total-pages="totalPages"
        :is-loading="isLoading"
        :items="paginationItems"
        jump-input-id="problem-page-jump"
        :jump-placeholder="`1-${totalPages}`"
        @page-change="goToPage"
        @jump-submit="submitPageJump"
      />
    </article>
  </section>
</template>

<script setup>
import PaginationBar from '@/components/PaginationBar.vue'
import ProblemsFilters from '@/components/problems/ProblemsFilters.vue'
import ProblemsTable from '@/components/problems/ProblemsTable.vue'
import ProblemsToolbar from '@/components/problems/ProblemsToolbar.vue'
import { useProblemBrowse } from '@/composables/useProblemBrowse'

const {
  searchState,
  isLoading,
  errorMessage,
  problemSortOptions,
  problemStateFilterOptions,
  showProblemStateFilters,
  hasAppliedTitleFilter,
  appliedSortKey,
  appliedSortDirection,
  appliedStateFilter,
  problemCount,
  pagedProblems,
  visibleRangeText,
  emptyStateMessage,
  currentPage,
  totalPages,
  paginationItems,
  submitSearch,
  resetSearch,
  cycleSort,
  applyStateFilter,
  goToPage,
  submitPageJump
} = useProblemBrowse()
</script>

<style scoped>
.problems-panel {
  display: grid;
  gap: 1.25rem;
}
</style>
