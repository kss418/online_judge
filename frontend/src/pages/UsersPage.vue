<template>
  <section class="page-grid single-column">
    <UserListTableShell
      v-model:search-input="searchInput"
      v-model:page-jump-input="pageJumpInput"
      kicker="directory"
      title="유저 목록"
      description="ID로 검색하고, 계정 번호와 제출 기록 요약을 확인할 수 있습니다."
      :status-label="isLoading ? 'Loading' : `${users.length} Users`"
      :status-tone="errorMessage ? 'danger' : 'success'"
      :applied-query="appliedQuery"
      search-input-id="user-search"
      search-placeholder="ID 검색"
      :show-refresh="true"
      :is-loading="isLoading"
      :view-state="viewState"
      :view-message="viewMessage"
      :empty-message="emptyMessage"
      :columns="tableColumns"
      :rows="pagedUsers"
      grid-template-columns="0.8fr 1.5fr 0.8fr 0.8fr 0.85fr 1.2fr"
      table-min-width="900px"
      row-intrinsic-size="74px"
      :current-page="currentPage"
      :total-pages="totalPages"
      :pagination-items="paginationItems"
      jump-input-id="users-page-jump"
      :jump-placeholder="`1-${totalPages}`"
      @submit-search="submitSearch"
      @reset-search="resetSearch"
      @refresh="loadUsers"
      @page-change="goToPage"
      @jump-submit="submitPageJump"
    >
      <template #row="{ row: user }">
        <strong>#{{ formatCount(user.user_id) }}</strong>
        <RouterLink
          class="users-link"
          :to="{ name: 'user-info', params: { userLoginId: user.user_login_id } }"
        >
          {{ user.user_login_id }}
        </RouterLink>
        <strong>{{ formatCount(user.solved_problem_count) }}</strong>
        <strong>{{ formatCount(user.submission_count) }}</strong>
        <strong class="users-acceptance-rate">
          {{ formatAcceptanceRate(user.accepted_submission_count, user.submission_count) }}
        </strong>
        <time
          class="users-created-at"
          :datetime="user.created_at"
        >
          <span class="users-relative-time" tabindex="0">
            {{ formatRelativeCreatedAt(user.created_at_timestamp) }}
            <span
              v-if="user.created_at_label"
              class="users-time-tooltip"
            >
              {{ user.created_at_label }}
            </span>
          </span>
        </time>
      </template>
    </UserListTableShell>
  </section>
</template>

<script setup>
import UserListTableShell from '@/components/users/UserListTableShell.vue'
import { useUsersPage } from '@/composables/useUsersPage'

const tableColumns = Object.freeze([
  '계정 번호',
  'ID',
  '맞은 문제',
  '제출 수',
  '정답률',
  '생성 시각'
])

const {
  formatCount,
  users,
  isLoading,
  errorMessage,
  searchInput,
  appliedQuery,
  currentPage,
  totalPages,
  pageJumpInput,
  paginationItems,
  pagedUsers,
  loadUsers,
  submitSearch,
  resetSearch,
  goToPage,
  submitPageJump,
  formatAcceptanceRate,
  formatRelativeCreatedAt,
  viewState,
  viewMessage,
  emptyMessage
} = useUsersPage()
</script>

<style scoped>
.users-link {
  min-width: 0;
  font-weight: 600;
  color: var(--ink-strong);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 160ms ease,
    text-decoration-color 160ms ease,
    transform 160ms ease;
}

.users-link:hover,
.users-link:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.users-acceptance-rate {
  color: var(--ink-strong);
}

.users-created-at {
  white-space: nowrap;
  font-size: 0.92rem;
  font-weight: 600;
  color: #6C8BCF;
}

.users-relative-time {
  position: relative;
  display: inline-flex;
  align-items: center;
  color: inherit;
  cursor: help;
  outline: none;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 140ms ease,
    text-decoration-color 140ms ease,
    transform 140ms ease;
}

.users-relative-time:hover,
.users-relative-time:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.users-time-tooltip {
  position: absolute;
  right: 0;
  bottom: calc(100% + 0.55rem);
  z-index: 10;
  min-width: max-content;
  max-width: 220px;
  padding: 0.55rem 0.7rem;
  border: 1px solid rgba(20, 33, 61, 0.12);
  border-radius: 12px;
  background: rgba(20, 33, 61, 0.96);
  box-shadow: 0 14px 32px rgba(20, 33, 61, 0.18);
  color: #f8fafc;
  font-size: 0.8rem;
  font-weight: 600;
  line-height: 1.4;
  white-space: nowrap;
  pointer-events: none;
  opacity: 0;
  transform: translateY(6px);
  transition:
    opacity 140ms ease,
    transform 140ms ease;
}

.users-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.users-relative-time:hover .users-time-tooltip,
.users-relative-time:focus-visible .users-time-tooltip {
  opacity: 1;
  transform: translateY(0);
}
</style>
