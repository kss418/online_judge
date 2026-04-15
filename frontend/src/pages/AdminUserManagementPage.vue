<template>
  <section class="page-grid single-column">
    <UserListTableShell
      v-model:search-input="searchInput"
      v-model:page-jump-input="pageJumpInput"
      kicker="operations"
      title="유저 관리"
      description="가입한 사용자를 확인하고 제출 금지 상태를 운영 관점에서 관리할 수 있습니다."
      :status-label="authState.isInitializing || isLoading ? 'Loading' : `${filteredUsers.length} Users`"
      :status-tone="errorMessage ? 'danger' : 'success'"
      :applied-query="appliedQuery"
      search-input-id="admin-user-management-search"
      search-placeholder="ID 검색"
      :show-refresh="true"
      :is-loading="isLoading"
      :view-state="viewState"
      :view-message="viewMessage"
      :empty-message="emptyMessage"
      :columns="tableColumns"
      :rows="pagedUsers"
      grid-template-columns="0.8fr 1.9fr 1.6fr 2.6fr"
      table-min-width="1120px"
      row-intrinsic-size="86px"
      :current-page="currentPage"
      :total-pages="totalPages"
      :pagination-items="paginationItems"
      jump-input-id="admin-user-management-page-jump"
      :jump-placeholder="`1-${totalPages}`"
      @submit-search="submitSearch"
      @reset-search="resetSearch"
      @refresh="loadUsers"
      @page-change="goToPage"
      @jump-submit="submitPageJump"
    >
      <template #summary>
        <div v-if="isAuthenticated && canManageUsers" class="admin-user-management-summary">
          <div class="metric-row">
            <span class="metric-label">전체 사용자</span>
            <strong>{{ users.length }}</strong>
          </div>
          <div class="metric-row">
            <span class="metric-label">정상</span>
            <strong>{{ normalUserCount }}</strong>
          </div>
          <div class="metric-row">
            <span class="metric-label">제출 금지</span>
            <strong>{{ activeBanCount }}</strong>
          </div>
        </div>
      </template>

      <template #row="{ row: user }">
        <strong>#{{ formatCount(user.user_id) }}</strong>

        <div class="admin-user-identity">
          <RouterLink
            class="admin-user-link"
            :to="{ name: 'user-info', params: { userLoginId: user.user_login_id } }"
          >
            <strong>{{ user.user_login_id || '-' }}</strong>
          </RouterLink>
        </div>

        <div class="admin-user-ban-status">
          <StatusBadge
            :label="getSubmissionBanLabel(user)"
            :tone="getSubmissionBanTone(user)"
          />
          <strong
            v-if="formatSubmissionBanWindow(user)"
            class="admin-user-ban-copy"
          >
            {{ formatSubmissionBanWindow(user) }}
          </strong>
          <span
            v-if="shouldShowSubmissionBanUntil(user)"
            class="admin-user-ban-until"
          >
            해제 시각 {{ user.submission_banned_until_label }}
          </span>
        </div>

        <div class="admin-user-ban-actions">
          <div class="admin-user-ban-presets">
            <button
              v-for="preset in durationPresets"
              :key="`${user.user_id}-${preset.minutes}`"
              type="button"
              class="ghost-button admin-user-ban-preset"
              :disabled="isBusyUser(user.user_id)"
              @click="setDurationDraft(user.user_id, preset.minutes)"
            >
              {{ preset.label }}
            </button>
          </div>

          <div class="admin-user-ban-form">
            <label class="sr-only" :for="`submission-ban-${user.user_id}`">
              제출 밴 분 단위
            </label>
            <input
              :id="`submission-ban-${user.user_id}`"
              :value="getDurationDraft(user.user_id)"
              class="admin-user-ban-input"
              type="number"
              min="1"
              step="1"
              inputmode="numeric"
              placeholder="분 단위"
              :disabled="isBusyUser(user.user_id)"
              @input="(event) => updateDurationDraft(user.user_id, event.target.value)"
            />
            <button
              type="button"
              class="primary-button admin-user-ban-submit"
              :disabled="isBusyUser(user.user_id)"
              @click="handleCreateSubmissionBan(user)"
            >
              {{ getCreateBanButtonLabel(user) }}
            </button>
            <button
              type="button"
              class="ghost-button admin-user-ban-clear"
              :disabled="isBusyUser(user.user_id) || !user.submission_banned_until"
              @click="handleClearSubmissionBan(user)"
            >
              {{ getClearBanButtonLabel(user) }}
            </button>
          </div>
        </div>
      </template>
    </UserListTableShell>
  </section>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'
import UserListTableShell from '@/components/users/UserListTableShell.vue'
import { useAdminUserManagementPage } from '@/composables/useAdminUserManagementPage'

const tableColumns = Object.freeze([
  '계정 번호',
  'ID',
  '제출 밴',
  '관리'
])

const {
  authState,
  isAuthenticated,
  canManageUsers,
  formatCount,
  durationPresets,
  isLoading,
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
  activeBanCount,
  normalUserCount,
  loadUsers,
  submitSearch,
  resetSearch,
  goToPage,
  submitPageJump,
  getSubmissionBanLabel,
  getSubmissionBanTone,
  formatSubmissionBanWindow,
  shouldShowSubmissionBanUntil,
  getDurationDraft,
  setDurationDraft,
  updateDurationDraft,
  handleCreateSubmissionBan,
  handleClearSubmissionBan,
  getCreateBanButtonLabel,
  getClearBanButtonLabel,
  isBusyUser,
  viewState,
  viewMessage,
  emptyMessage
} = useAdminUserManagementPage()
</script>

<style scoped>
.admin-user-management-summary {
  display: grid;
  gap: 0.85rem;
  grid-template-columns: repeat(3, minmax(0, 1fr));
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

.admin-user-identity {
  display: grid;
  gap: 0.25rem;
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

.admin-user-ban-status {
  display: grid;
  gap: 0.3rem;
}

.admin-user-ban-status :deep(.status-badge) {
  justify-self: start;
  width: fit-content;
  min-height: 1.85rem;
  padding: 0.28rem 0.68rem;
}

.admin-user-ban-copy {
  font-size: 0.94rem;
}

.admin-user-ban-until {
  color: var(--ink-soft);
  font-size: 0.84rem;
}

.admin-user-ban-actions {
  display: grid;
  gap: 0.7rem;
}

.admin-user-ban-presets {
  display: flex;
  flex-wrap: wrap;
  gap: 0.55rem;
}

.admin-user-ban-preset {
  min-height: 2.2rem;
  padding-inline: 0.85rem;
}

.admin-user-ban-form {
  display: flex;
  flex-wrap: wrap;
  gap: 0.6rem;
  align-items: center;
}

.admin-user-ban-input {
  width: 8rem;
  min-height: 2.75rem;
  padding: 0.7rem 0.85rem;
  border: 1px solid rgba(20, 33, 61, 0.12);
  border-radius: 14px;
  background: rgba(255, 255, 255, 0.84);
  color: var(--ink-strong);
}

.admin-user-ban-submit,
.admin-user-ban-clear {
  min-height: 2.75rem;
  padding-inline: 1rem;
  white-space: nowrap;
}

.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}

@media (max-width: 1180px) {
  .admin-user-management-summary {
    grid-template-columns: minmax(0, 1fr);
  }
}
</style>
