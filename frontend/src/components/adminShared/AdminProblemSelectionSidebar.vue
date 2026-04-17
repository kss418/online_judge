<template>
  <AdminSelectionSidebarShell
    :list-caption="sidebar.model.problemListCaption"
    :item-count="sidebar.model.problemCount"
    :is-loading="sidebar.model.isLoadingProblems"
    :error-message="sidebar.model.listErrorMessage"
    :empty-message="sidebar.model.emptyProblemListMessage"
    loading-message="문제 목록을 불러오는 중입니다."
    :format-count="sidebar.model.formatCount"
  >
    <template #search>
      <form class="admin-problem-selection-sidebar__search" @submit.prevent="sidebar.actions.submitSearch()">
        <div
          class="admin-problem-selection-sidebar__mode-switch"
          role="tablist"
          aria-label="문제 검색 방식"
        >
          <button
            type="button"
            class="ghost-button admin-problem-selection-sidebar__mode-button"
            :class="{ 'is-active': sidebar.model.searchMode === 'title' }"
            :aria-pressed="sidebar.model.searchMode === 'title'"
            @click="sidebar.actions.setSearchMode('title')"
          >
            제목 검색
          </button>
          <button
            type="button"
            class="ghost-button admin-problem-selection-sidebar__mode-button"
            :class="{ 'is-active': sidebar.model.searchMode === 'problem-id' }"
            :aria-pressed="sidebar.model.searchMode === 'problem-id'"
            @click="sidebar.actions.setSearchMode('problem-id')"
          >
            번호 검색
          </button>
        </div>
        <input
          v-if="sidebar.model.searchMode === 'title'"
          :id="sidebar.model.titleSearchInputId"
          :value="sidebar.model.titleSearchInput"
          class="field-input admin-problem-selection-sidebar__search-input"
          type="search"
          aria-label="문제 제목 검색"
          placeholder="문제 제목 검색"
          @input="handleTitleSearchInput"
        />
        <input
          v-else
          :id="sidebar.model.problemIdSearchInputId"
          :value="sidebar.model.problemIdSearchInput"
          class="field-input admin-problem-selection-sidebar__search-input"
          type="text"
          inputmode="numeric"
          aria-label="문제 번호 검색"
          placeholder="예: 123"
          @input="handleProblemIdSearchInput"
        />
        <button
          type="submit"
          class="ghost-button"
          :disabled="sidebar.model.isLoadingProblems || Boolean(sidebar.model.busySection)"
        >
          검색
        </button>
        <button
          v-if="sidebar.model.hasAppliedSearch"
          type="button"
          class="ghost-button"
          :disabled="sidebar.model.isLoadingProblems || Boolean(sidebar.model.busySection)"
          @click="sidebar.actions.resetSearch()"
        >
          초기화
        </button>
      </form>
    </template>

    <template #before-list>
      <form
        v-if="sidebar.create"
        class="admin-problems-create"
        @submit.prevent="handleCreateProblem"
      >
        <div>
          <p class="panel-kicker">create</p>
          <h3>새 문제</h3>
        </div>

        <div class="admin-problems-create-row">
          <input
            :value="sidebar.create.model.newProblemTitle"
            class="field-input"
            type="text"
            maxlength="120"
            placeholder="새 문제 제목"
            @input="handleNewProblemTitleInput"
          />
        </div>

        <div class="admin-problems-create-actions">
          <button
            type="submit"
            class="primary-button"
            :disabled="!sidebar.create.model.canCreateProblem"
          >
            {{ sidebar.create.model.isCreatingProblem ? '생성 중...' : '문제 생성' }}
          </button>
        </div>
      </form>
    </template>

    <button
      v-for="problem in sidebar.model.problems"
      :key="problem.problem_id"
      type="button"
      class="admin-problem-selection-sidebar__item"
      :class="{ 'is-active': problem.problem_id === sidebar.model.selectedProblemId }"
      @click="sidebar.actions.selectProblem(problem.problem_id)"
    >
      <div class="admin-problem-selection-sidebar__item-head">
        <strong>#{{ sidebar.model.formatCount(problem.problem_id) }}</strong>
        <span class="admin-problem-selection-sidebar__item-version">v{{ problem.version }}</span>
      </div>
      <strong class="admin-problem-selection-sidebar__item-title">{{ problem.title }}</strong>
      <div class="admin-problem-selection-sidebar__item-limits">
        <span>{{ sidebar.model.formatProblemLimit(problem.time_limit_ms, 'ms') }}</span>
        <span>{{ sidebar.model.formatProblemLimit(problem.memory_limit_mb, 'MB') }}</span>
      </div>
    </button>
  </AdminSelectionSidebarShell>
</template>

<script setup>
import { computed } from 'vue'

import AdminSelectionSidebarShell from '@/components/adminShared/AdminSelectionSidebarShell.vue'

const props = defineProps({
  sidebar: {
    type: Object,
    required: true
  }
})

const sidebar = computed(() => props.sidebar)

function handleTitleSearchInput(event){
  sidebar.value.actions.updateTitleSearchInput(event.target.value)
}

function handleProblemIdSearchInput(event){
  sidebar.value.actions.updateProblemIdSearchInput(event.target.value)
}

function handleNewProblemTitleInput(event){
  sidebar.value.create?.actions.updateNewProblemTitle(event.target.value)
}

function handleCreateProblem(){
  sidebar.value.create?.actions.createProblem()
}
</script>

<style scoped>
.admin-problem-selection-sidebar__search {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-selection-sidebar__mode-switch {
  display: inline-flex;
  gap: 0.5rem;
  padding: 0.3rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.88);
}

.admin-problem-selection-sidebar__mode-button {
  border-radius: 999px;
}

.admin-problem-selection-sidebar__mode-button.is-active {
  color: var(--warning);
  background: rgba(255, 247, 237, 0.98);
  border-color: rgba(217, 119, 6, 0.24);
}

.admin-problem-selection-sidebar__search-input {
  flex: 1 1 220px;
}

.admin-problems-create {
  display: grid;
  gap: 0.9rem;
  padding: 1rem;
  border-radius: 20px;
  border: 1px solid var(--admin-workspace-section-border);
  background: var(--admin-workspace-section-surface);
  box-shadow: var(--admin-workspace-section-shadow);
}

.admin-problems-create-row {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problems-create-row .field-input {
  flex: 1 1 220px;
}

.admin-problems-create-actions {
  display: flex;
  justify-content: flex-end;
}

.admin-problem-selection-sidebar__item {
  appearance: none;
  width: 100%;
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 1rem;
  border-radius: 18px;
  border: 1px solid var(--admin-workspace-nested-border);
  background: var(--admin-workspace-nested-surface);
  text-align: left;
  font: inherit;
  color: var(--ink-strong);
  cursor: pointer;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    background 160ms ease,
    box-shadow 160ms ease;
}

.admin-problem-selection-sidebar__item:hover {
  transform: translateY(-1px);
}

.admin-problem-selection-sidebar__item:hover,
.admin-problem-selection-sidebar__item.is-active {
  border-color: rgba(217, 119, 6, 0.32);
  background: rgba(255, 251, 235, 0.98);
  box-shadow: 0 12px 28px rgba(217, 119, 6, 0.12);
}

.admin-problem-selection-sidebar__item-head,
.admin-problem-selection-sidebar__item-limits {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
}

.admin-problem-selection-sidebar__item-head {
  color: var(--ink-soft);
  font-size: 0.86rem;
}

.admin-problem-selection-sidebar__item-title {
  display: block;
  font-size: 1rem;
  line-height: 1.45;
}

.admin-problem-selection-sidebar__item-version,
.admin-problem-selection-sidebar__item-limits {
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 600;
}

@media (max-width: 720px) {
  .admin-problem-selection-sidebar__search {
    display: grid;
    align-items: stretch;
  }

  .admin-problems-create-row {
    display: grid;
    align-items: stretch;
  }

  .admin-problem-selection-sidebar__item-head,
  .admin-problem-selection-sidebar__item-limits {
    align-items: start;
    flex-direction: column;
  }
}
</style>
