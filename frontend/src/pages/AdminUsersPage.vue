<template>
  <section class="page-grid single-column">
    <UserListTableShell
      v-model:search-input="searchInput"
      v-model:page-jump-input="pageJumpInput"
      kicker="Privilege"
      title="권한 관리"
      description="가입한 사용자를 확인하고 어드민 승격과 유저 강등을 관리할 수 있습니다."
      :status-label="isBusy ? 'Loading' : `${filteredUsers.length} Users`"
      :status-tone="errorMessage ? 'danger' : 'success'"
      :applied-query="appliedQuery"
      search-input-id="admin-users-search"
      search-placeholder="ID 검색"
      :show-refresh="canManageUsers"
      :is-loading="isLoading"
      :view-state="viewState"
      :view-message="viewMessage"
      :empty-message="emptyMessage"
      :columns="tableColumns"
      :rows="pagedUsers"
      grid-template-columns="0.8fr 1.9fr 1fr 1.2fr"
      table-min-width="760px"
      row-intrinsic-size="74px"
      :current-page="currentPage"
      :total-pages="totalPages"
      :pagination-items="paginationItems"
      jump-input-id="admin-users-page-jump"
      :jump-placeholder="`1-${totalPages}`"
      @submit-search="submitSearch"
      @reset-search="resetSearch"
      @refresh="loadUsers"
      @page-change="goToPage"
      @jump-submit="submitPageJump"
    >
      <template #summary>
        <div v-if="viewState === 'ready'" class="admin-users-summary">
          <div class="metric-row">
            <span class="metric-label">전체 사용자</span>
            <strong>{{ users.length }}</strong>
          </div>
          <div class="metric-row">
            <span class="metric-label">SuperAdmin</span>
            <strong>{{ superAdminCount }}</strong>
          </div>
          <div class="metric-row">
            <span class="metric-label">Admin</span>
            <strong>{{ adminCount }}</strong>
          </div>
          <div class="metric-row">
            <span class="metric-label">User</span>
            <strong>{{ regularUserCount }}</strong>
          </div>
        </div>
      </template>

      <template #row="{ row: user }">
        <strong>#{{ user.user_id }}</strong>
        <div class="admin-user-name">
          <RouterLink
            class="admin-user-link"
            :to="{ name: 'user-info', params: { userLoginId: user.user_login_id } }"
          >
            <strong>{{ user.user_login_id || '-' }}</strong>
          </RouterLink>
        </div>
        <div class="admin-user-role">
          <StatusBadge
            :label="formatPermissionLabel(user.permission_level)"
            :tone="getPermissionTone(user.permission_level)"
          />
        </div>
        <div class="admin-user-actions">
          <button
            v-if="user.permission_level === 0"
            type="button"
            class="ghost-button admin-user-action-button"
            :disabled="!canEditPermissions || savingUserId === user.user_id"
            @click="handlePromoteToAdmin(user)"
          >
            {{ savingUserId === user.user_id ? '처리 중...' : '어드민 승격' }}
          </button>
          <button
            v-else-if="user.permission_level === 1"
            type="button"
            class="ghost-button admin-user-action-button"
            :disabled="!canEditPermissions || savingUserId === user.user_id"
            @click="handleDemoteToUser(user)"
          >
            {{ savingUserId === user.user_id ? '처리 중...' : '유저 강등' }}
          </button>
          <span v-else class="admin-user-action-placeholder" aria-hidden="true"></span>
        </div>
      </template>
    </UserListTableShell>
  </section>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'
import UserListTableShell from '@/components/users/UserListTableShell.vue'
import { useAdminUsersPage } from '@/composables/useAdminUsersPage'

const tableColumns = Object.freeze([
  '계정 번호',
  'ID',
  '권한',
  '관리'
])

const {
  canManageUsers,
  canEditPermissions,
  isLoading,
  isBusy,
  errorMessage,
  users,
  filteredUsers,
  pagedUsers,
  searchInput,
  appliedQuery,
  currentPage,
  totalPages,
  pageJumpInput,
  paginationItems,
  savingUserId,
  superAdminCount,
  adminCount,
  regularUserCount,
  loadUsers,
  submitSearch,
  resetSearch,
  goToPage,
  submitPageJump,
  handlePromoteToAdmin,
  handleDemoteToUser,
  formatPermissionLabel,
  getPermissionTone,
  viewState,
  viewMessage,
  emptyMessage
} = useAdminUsersPage()
</script>

<style scoped>
.admin-users-summary {
  display: grid;
  gap: 0.85rem;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  margin-bottom: 1rem;
}

.metric-row {
  display: grid;
  gap: 0.35rem;
  padding: 0.9rem 1rem;
  border: 1px solid rgba(20, 33, 61, 0.1);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.74);
}

.metric-label {
  color: var(--ink-soft);
  font-size: 0.85rem;
  font-weight: 700;
}

.admin-user-name {
  display: grid;
  gap: 0.2rem;
}

.admin-user-link {
  width: fit-content;
  color: inherit;
  text-decoration: none;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 140ms ease,
    text-decoration-color 140ms ease,
    transform 140ms ease;
}

.admin-user-link:hover,
.admin-user-link:focus-visible {
  color: #1D4ED8;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.admin-user-role {
  display: grid;
  gap: 0.3rem;
  justify-items: start;
}

.admin-user-actions {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 0.65rem;
}

.admin-user-action-button {
  min-height: 2.4rem;
  padding-inline: 0.95rem;
  white-space: nowrap;
}

.admin-user-action-placeholder {
  display: inline-block;
  min-height: 2.4rem;
}

@media (max-width: 1120px) {
  .admin-users-summary {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
