<template>
  <section class="page-grid single-column">
    <article class="panel admin-user-management-panel">
      <div class="admin-user-management-toolbar">
        <div>
          <p class="panel-kicker">operations</p>
          <h3>유저 관리</h3>
          <p class="admin-user-management-copy">
            가입한 사용자를 확인하고 제출 금지 상태를 운영 관점에서 관리할 수 있습니다.
          </p>
        </div>

        <div class="admin-user-management-actions">
          <StatusBadge
            :label="isLoading ? 'Loading' : `${filteredUsers.length} Users`"
            :tone="errorMessage ? 'danger' : 'success'"
          />

          <div class="admin-user-search-row">
            <form class="admin-user-search" @submit.prevent="submitSearch">
              <label class="sr-only" for="admin-user-management-search">유저 검색</label>
              <input
                id="admin-user-management-search"
                v-model.trim="searchInput"
                class="admin-user-search-input"
                type="search"
                placeholder="ID 검색"
              />
              <button
                type="submit"
                class="primary-button admin-user-search-button"
                :disabled="isLoading"
              >
                검색
              </button>
            </form>

            <button
              v-if="appliedQuery"
              type="button"
              class="ghost-button admin-user-reset-button"
              :disabled="isLoading"
              @click="resetSearch"
            >
              검색 초기화
            </button>
          </div>

          <button
            type="button"
            class="ghost-button"
            :disabled="isLoading"
            @click="loadUsers"
          >
            새로고침
          </button>
        </div>
      </div>

      <div v-if="authState.isInitializing" class="empty-state">
        <p>관리자 권한을 확인하는 중입니다.</p>
      </div>

      <div v-else-if="!isAuthenticated" class="empty-state">
        <p>유저 관리 페이지는 로그인한 관리자만 사용할 수 있습니다.</p>
      </div>

      <div v-else-if="!canManageUsers" class="empty-state error-state">
        <p>이 페이지는 관리자만 접근할 수 있습니다.</p>
      </div>

      <template v-else>
        <div class="admin-user-management-summary">
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

        <div v-if="isLoading" class="empty-state">
          <p>유저 목록을 불러오는 중입니다.</p>
        </div>

        <div v-else-if="errorMessage" class="empty-state error-state">
          <p>{{ errorMessage }}</p>
        </div>

        <div v-else-if="!filteredUsers.length" class="empty-state">
          <p>{{ appliedQuery ? '검색 결과가 없습니다.' : '표시할 사용자가 아직 없습니다.' }}</p>
        </div>

        <div v-else class="admin-user-management-table">
          <div class="admin-user-management-head">
            <span>계정 번호</span>
            <span>ID</span>
            <span>제출 밴</span>
            <span>관리</span>
          </div>

          <div
            v-for="user in pagedUsers"
            :key="user.user_id"
            class="admin-user-management-row"
          >
            <strong>#{{ countFormatter.format(user.user_id) }}</strong>

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
          </div>
        </div>

        <PaginationBar
          v-if="!isLoading && !errorMessage && filteredUsers.length > pageSize"
          v-model:jump-input="pageJumpInput"
          :current-page="currentPage"
          :total-pages="totalPages"
          :is-loading="isLoading"
          :items="paginationItems"
          jump-input-id="admin-user-management-page-jump"
          :jump-placeholder="`1-${totalPages}`"
          @page-change="goToPage"
          @jump-submit="submitPageJump"
        />
      </template>
    </article>
  </section>
</template>

<script setup>
import PaginationBar from '@/components/PaginationBar.vue'
import StatusBadge from '@/components/StatusBadge.vue'
import { useAdminUserManagementPage } from '@/composables/useAdminUserManagementPage'

const {
  authState,
  isAuthenticated,
  canManageUsers,
  countFormatter,
  pageSize,
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
  isBusyUser
} = useAdminUserManagementPage()
</script>

<style scoped>
.admin-user-management-panel {
  display: grid;
  gap: 1rem;
}

.admin-user-management-toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-user-management-copy {
  margin: 0.45rem 0 0;
  color: var(--ink-soft);
}

.admin-user-management-actions {
  display: flex;
  flex-wrap: wrap;
  justify-content: end;
  align-items: center;
  gap: 0.75rem;
}

.admin-user-search-row {
  display: flex;
  justify-content: flex-end;
  gap: 1rem;
  align-items: center;
  flex-wrap: wrap;
  min-width: 0;
}

.admin-user-search {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  min-width: 0;
}

.admin-user-search-input {
  width: min(100%, 300px);
  min-height: 2.9rem;
  padding: 0.8rem 0.95rem;
  border-radius: 999px;
  border: 1px solid rgba(20, 33, 61, 0.14);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.admin-user-search-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-user-search-button,
.admin-user-reset-button {
  flex-shrink: 0;
}

.admin-user-management-summary {
  display: grid;
  gap: 0.85rem;
  grid-template-columns: repeat(3, minmax(0, 1fr));
}

.admin-user-management-table {
  border: 1px solid var(--line);
  border-radius: 22px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.72);
}

.admin-user-management-head,
.admin-user-management-row {
  display: grid;
  grid-template-columns: 0.8fr 1.9fr 1.6fr 2.6fr;
  gap: 1rem;
  align-items: center;
  padding: 1rem 1.15rem;
}

.admin-user-management-head {
  font-size: 0.88rem;
  font-weight: 700;
  color: var(--ink-soft);
  background: rgba(20, 33, 61, 0.04);
  border-bottom: 1px solid var(--line);
}

.admin-user-management-row + .admin-user-management-row {
  border-top: 1px solid var(--line);
}

.admin-user-management-row {
  content-visibility: auto;
  contain-intrinsic-size: 86px;
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

.admin-user-created-at {
  white-space: nowrap;
  font-size: 0.92rem;
  font-weight: 600;
  color: #6C8BCF;
}

.admin-user-time-chip {
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

.admin-user-time-chip:hover,
.admin-user-time-chip:focus-visible {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.admin-user-time-tooltip {
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

.admin-user-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.admin-user-time-chip:hover .admin-user-time-tooltip,
.admin-user-time-chip:focus-visible .admin-user-time-tooltip {
  opacity: 1;
  transform: translateY(0);
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

@media (max-width: 1180px) {
  .admin-user-management-toolbar {
    flex-direction: column;
  }

  .admin-user-management-actions {
    justify-content: flex-start;
  }

  .admin-user-search-row,
  .admin-user-search {
    justify-content: flex-start;
  }

  .admin-user-management-summary {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-user-management-table {
    overflow-x: auto;
  }

  .admin-user-management-head,
  .admin-user-management-row {
    min-width: 1120px;
  }
}

@media (max-width: 720px) {
  .admin-user-search-input {
    min-width: 0;
    width: 100%;
  }
}
</style>
