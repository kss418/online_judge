<template>
  <section class="page-grid single-column">
    <div
      class="my-info-layout"
      :class="{ 'is-summary-only': !showExtendedProfilePanels }"
    >
      <div v-if="showExtendedProfilePanels" class="my-info-side-column">
        <article class="panel my-info-panel my-info-statistics-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">statistics</p>
              <h3>제출 통계</h3>
            </div>

            <StatusBadge
              :label="statisticsStatusLabel"
              :tone="statisticsStatusTone"
            />
          </div>

          <div v-if="shouldShowLoadingState || isStatisticsLoading" class="empty-state">
            <p>제출 통계를 불러오는 중입니다.</p>
          </div>

          <div v-else-if="statisticsErrorMessage" class="empty-state error-state">
            <p>{{ statisticsErrorMessage }}</p>
          </div>

          <div v-else class="my-info-summary">
            <div
              v-for="item in statisticsItems"
              :key="item.label"
              class="metric-row"
            >
              <SubmissionStatusBadge
                v-if="item.status"
                :status="item.status"
              />
              <span v-else class="metric-label">
                {{ item.label }}
              </span>
              <strong>{{ item.value }}</strong>
            </div>
          </div>
        </article>

        <article class="panel my-info-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">recent</p>
              <h3>최근 제출 목록</h3>
            </div>
          </div>

          <div v-if="shouldShowLoadingState || isRecentSubmissionsLoading" class="empty-state">
            <p>최근 제출 목록을 불러오는 중입니다.</p>
          </div>

          <div v-else-if="recentSubmissionsErrorMessage" class="empty-state error-state">
            <p>{{ recentSubmissionsErrorMessage }}</p>
          </div>

          <div v-else-if="recentSubmissions.length === 0" class="empty-state">
            <p>아직 제출한 기록이 없습니다.</p>
          </div>

          <div v-else class="recent-submission-list">
            <RouterLink
              v-for="submission in recentSubmissions"
              :key="submission.submission_id"
              class="recent-submission-item"
              :to="{ name: 'problem-detail', params: { problemId: submission.problem_id } }"
            >
              <span class="recent-submission-item__problem-heading">
                <span class="recent-submission-item__problem-id-group">
                  <span class="recent-submission-item__problem-id">
                    #{{ submission.problem_id }}
                  </span>
                  <span
                    class="recent-submission-item__problem-divider"
                    aria-hidden="true"
                  >
                    ·
                  </span>
                </span>
                <span class="recent-submission-item__problem-title">
                  {{ submission.problem_title }}
                </span>
              </span>
              <SubmissionStatusBadge :status="submission.status" />
              <span class="recent-submission-item__time">
                <span class="recent-submission-relative-time">
                  {{ formatRelativeSubmittedAt(
                    submission.created_at_timestamp,
                    submission.created_at_label
                  ) }}
                  <span
                    v-if="submission.created_at_label"
                    class="recent-submission-time-tooltip"
                  >
                    {{ submission.created_at_label }}
                  </span>
                </span>
              </span>
            </RouterLink>
          </div>
        </article>
      </div>

      <div class="my-info-main-column">
        <article class="panel my-info-panel">
          <div class="panel-header">
            <div>
              <p class="panel-kicker">account</p>
              <h3>정보</h3>
            </div>

            <StatusBadge
              :label="profileStatusLabel"
              :tone="profileStatusTone"
            />
          </div>

          <div v-if="isProfileLoading" class="empty-state">
            <p>정보를 확인하는 중입니다.</p>
          </div>

          <div v-else-if="profileErrorMessage" class="empty-state error-state">
            <p>{{ profileErrorMessage }}</p>
          </div>

          <div v-else-if="!showExtendedProfilePanels" class="empty-state">
            <p>로그인하면 계정 기본 정보를 여기서 확인할 수 있습니다.</p>
          </div>

          <div v-else class="my-info-summary">
            <div class="metric-row">
              <span class="metric-label">계정 번호</span>
              <strong>#{{ displayedUser.user_id }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">ID</span>
              <strong>{{ displayedUser.user_login_id }}</strong>
            </div>
            <div class="metric-row">
              <span class="metric-label">만든 시각</span>
              <strong>{{ formatTimestamp(displayedUser.created_at) }}</strong>
            </div>
            <div v-if="isOwnProfile" class="metric-row">
              <span class="metric-label">제출 상태</span>
              <span class="my-info-role">
                <StatusBadge
                  :label="mySubmissionBanStatusLabel"
                  :tone="mySubmissionBanStatusTone"
                />
              </span>
            </div>
            <div v-if="isOwnProfile" class="metric-row">
              <span class="metric-label">제출 제한</span>
              <strong>{{ mySubmissionBanWindowText }}</strong>
            </div>
            <div v-if="isOwnProfile && mySubmissionBan.submission_banned_until" class="metric-row">
              <span class="metric-label">제출 가능 시각</span>
              <strong>{{ mySubmissionBanUntilText }}</strong>
            </div>
          </div>
        </article>

        <article
          v-if="showExtendedProfilePanels"
          class="panel my-info-panel"
        >
          <div class="panel-header">
            <div>
              <p class="panel-kicker">solved</p>
              <h3>푼 문제 목록</h3>
            </div>

            <StatusBadge
              :label="solvedProblemsStatusLabel"
              :tone="solvedProblemsStatusTone"
            />
          </div>

          <div v-if="shouldShowLoadingState || isSolvedProblemsLoading" class="empty-state">
            <p>푼 문제 목록을 불러오는 중입니다.</p>
          </div>

          <div v-else-if="solvedProblemsErrorMessage" class="empty-state error-state">
            <p>{{ solvedProblemsErrorMessage }}</p>
          </div>

          <div v-else-if="solvedProblems.length === 0" class="empty-state">
            <p>아직 푼 문제가 없습니다.</p>
          </div>

          <div v-else class="solved-problem-grid">
            <RouterLink
              v-for="problem in solvedProblems"
              :key="problem.problem_id"
              class="solved-problem-chip"
              :to="{ name: 'problem-detail', params: { problemId: problem.problem_id } }"
            >
              <strong
                class="solved-problem-chip__id"
                :class="getProblemStateTextClass(problem.user_problem_state)"
              >
                #{{ problem.problem_id }}
              </strong>
              <div class="solved-problem-chip__meta">
                <span class="solved-problem-chip__label">정답</span>
                <strong>{{ problem.accepted_count }}</strong>
              </div>
            </RouterLink>
          </div>
        </article>

        <article
          v-if="showExtendedProfilePanels"
          class="panel my-info-panel"
        >
          <div class="panel-header">
            <div>
              <p class="panel-kicker">wrong</p>
              <h3>틀린 문제 목록</h3>
            </div>

            <StatusBadge
              :label="wrongProblemsStatusLabel"
              :tone="wrongProblemsStatusTone"
            />
          </div>

          <div v-if="shouldShowLoadingState || isWrongProblemsLoading" class="empty-state">
            <p>틀린 문제 목록을 불러오는 중입니다.</p>
          </div>

          <div v-else-if="wrongProblemsErrorMessage" class="empty-state error-state">
            <p>{{ wrongProblemsErrorMessage }}</p>
          </div>

          <div v-else-if="wrongProblems.length === 0" class="empty-state">
            <p>아직 틀린 문제 목록이 없습니다.</p>
          </div>

          <div v-else class="wrong-problem-grid">
            <RouterLink
              v-for="problem in wrongProblems"
              :key="problem.problem_id"
              class="wrong-problem-chip"
              :to="{ name: 'problem-detail', params: { problemId: problem.problem_id } }"
            >
              <strong
                class="wrong-problem-chip__id"
                :class="getProblemStateTextClass(problem.user_problem_state)"
              >
                #{{ problem.problem_id }}
              </strong>
              <div class="wrong-problem-chip__meta">
                <span class="wrong-problem-chip__label">정답</span>
                <strong>{{ problem.accepted_count }}</strong>
              </div>
            </RouterLink>
          </div>
        </article>

      </div>
    </div>
  </section>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'
import SubmissionStatusBadge from '@/components/submissions/SubmissionStatusBadge.vue'
import { useMyInfoPage } from '@/composables/useMyInfoPage'

const {
  shouldShowLoadingState,
  isStatisticsLoading,
  statisticsErrorMessage,
  statisticsItems,
  statisticsStatusLabel,
  statisticsStatusTone,
  recentSubmissions,
  isRecentSubmissionsLoading,
  recentSubmissionsErrorMessage,
  isProfileLoading,
  profileErrorMessage,
  displayedUser,
  isOwnProfile,
  mySubmissionBan,
  mySubmissionBanStatusLabel,
  mySubmissionBanStatusTone,
  mySubmissionBanWindowText,
  mySubmissionBanUntilText,
  showExtendedProfilePanels,
  solvedProblems,
  isSolvedProblemsLoading,
  solvedProblemsErrorMessage,
  solvedProblemsStatusLabel,
  solvedProblemsStatusTone,
  wrongProblems,
  isWrongProblemsLoading,
  wrongProblemsErrorMessage,
  wrongProblemsStatusLabel,
  wrongProblemsStatusTone,
  profileStatusLabel,
  profileStatusTone,
  formatRelativeSubmittedAt,
  formatTimestamp,
  getProblemStateTextClass
} = useMyInfoPage()
</script>

<style scoped>
.my-info-layout {
  display: grid;
  grid-template-columns: minmax(364px, 468px) minmax(0, 1fr);
  gap: 1.25rem;
  align-items: start;
}

.my-info-layout.is-summary-only {
  grid-template-columns: minmax(0, 1fr);
}

.my-info-side-column {
  display: grid;
  gap: 1.25rem;
  align-content: start;
}

.my-info-main-column {
  display: grid;
  gap: 1.25rem;
  align-content: start;
}

.my-info-panel {
  display: grid;
  gap: 1.25rem;
}

.my-info-statistics-panel {
  align-self: start;
}

.my-info-copy {
  margin: 0.5rem 0 0;
  color: var(--text-muted);
  line-height: 1.6;
}

.my-info-summary {
  display: grid;
  gap: 0.85rem;
}

.my-info-role {
  display: inline-flex;
  align-items: center;
  justify-content: flex-end;
}

.recent-submission-list {
  display: grid;
  gap: 0.75rem;
}

.recent-submission-item {
  display: flex;
  align-items: center;
  gap: 0.8rem;
  padding: 0.95rem 1rem;
  border-radius: 16px;
  border: 1px solid rgba(148, 163, 184, 0.18);
  background:
    linear-gradient(180deg, rgba(15, 23, 42, 0.015), rgba(15, 23, 42, 0.04)),
    rgba(255, 255, 255, 0.96);
  text-decoration: none;
  color: inherit;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    box-shadow 160ms ease;
}

.recent-submission-item:hover,
.recent-submission-item:focus-visible {
  transform: translateY(-1px);
  border-color: rgba(37, 99, 235, 0.28);
  box-shadow: 0 18px 30px -28px rgba(37, 99, 235, 0.5);
}

.recent-submission-item__problem-heading {
  display: flex;
  align-items: baseline;
  gap: 0.75rem;
  min-width: 0;
  flex: 1 1 auto;
  font-size: 1rem;
  line-height: 1.12;
  letter-spacing: -0.02em;
}

.recent-submission-item__problem-id-group {
  display: inline-flex;
  align-items: baseline;
  gap: 0.5rem;
  white-space: nowrap;
  color: var(--ink-soft);
  flex-shrink: 0;
}

.recent-submission-item__problem-id {
  font-weight: 800;
  letter-spacing: -0.03em;
}

.recent-submission-item__problem-divider {
  color: rgba(20, 33, 61, 0.34);
  font-weight: 700;
}

.recent-submission-item__problem-title {
  font-weight: 700;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--ink-strong);
}

.recent-submission-item__time {
  color: #6C8BCF;
  font-size: 0.9rem;
  font-weight: 600;
  white-space: nowrap;
}

.recent-submission-relative-time {
  position: relative;
  display: inline-flex;
  align-items: center;
  color: inherit;
  cursor: help;
  text-decoration: underline dotted transparent;
  text-underline-offset: 0.18em;
  transition:
    color 140ms ease,
    text-decoration-color 140ms ease,
    transform 140ms ease;
}

.recent-submission-relative-time:hover {
  color: #4E6FB2;
  text-decoration-color: currentColor;
  transform: translateY(-1px);
}

.recent-submission-time-tooltip {
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

.recent-submission-time-tooltip::after {
  content: '';
  position: absolute;
  right: 0.8rem;
  top: 100%;
  border-width: 6px 6px 0;
  border-style: solid;
  border-color: rgba(20, 33, 61, 0.96) transparent transparent;
}

.recent-submission-relative-time:hover .recent-submission-time-tooltip {
  opacity: 1;
  transform: translateY(0);
}

.solved-problem-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(116px, 1fr));
  gap: 0.8rem;
  max-height: 28rem;
  overflow: auto;
  padding-right: 0.2rem;
}

.solved-problem-chip {
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 0.95rem;
  border-radius: 16px;
  border: 1px solid rgba(148, 163, 184, 0.22);
  background:
    linear-gradient(180deg, rgba(15, 23, 42, 0.015), rgba(15, 23, 42, 0.04)),
    rgba(255, 255, 255, 0.96);
  text-decoration: none;
  color: inherit;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    box-shadow 160ms ease;
}

.solved-problem-chip:hover,
.solved-problem-chip:focus-visible {
  transform: translateY(-1px);
  border-color: rgba(34, 197, 94, 0.34);
  box-shadow: 0 18px 30px -28px rgba(34, 197, 94, 0.55);
}

.solved-problem-chip__id {
  color: var(--ink-strong);
  font-size: 1rem;
  font-weight: 800;
}

.solved-problem-chip__id.problem-state-text--solved {
  color: #15803d;
}

.solved-problem-chip__id.problem-state-text--wrong {
  color: #b91c1c;
}

.solved-problem-chip__meta {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 0.65rem;
  color: var(--text-muted);
  font-size: 0.88rem;
}

.solved-problem-chip__label {
  color: var(--text-muted);
}

.wrong-problem-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(116px, 1fr));
  gap: 0.8rem;
  max-height: 28rem;
  overflow: auto;
  padding-right: 0.2rem;
}

.wrong-problem-chip {
  display: grid;
  gap: 0.55rem;
  padding: 0.9rem 0.95rem;
  border-radius: 16px;
  border: 1px solid rgba(148, 163, 184, 0.22);
  background:
    linear-gradient(180deg, rgba(15, 23, 42, 0.015), rgba(15, 23, 42, 0.04)),
    rgba(255, 255, 255, 0.96);
  text-decoration: none;
  color: inherit;
  transition:
    transform 160ms ease,
    border-color 160ms ease,
    box-shadow 160ms ease;
}

.wrong-problem-chip:hover,
.wrong-problem-chip:focus-visible {
  transform: translateY(-1px);
  border-color: rgba(239, 68, 68, 0.34);
  box-shadow: 0 18px 30px -28px rgba(239, 68, 68, 0.55);
}

.wrong-problem-chip__id {
  color: var(--ink-strong);
  font-size: 1rem;
  font-weight: 800;
}

.wrong-problem-chip__id.problem-state-text--solved {
  color: #15803d;
}

.wrong-problem-chip__id.problem-state-text--wrong {
  color: #b91c1c;
}

.wrong-problem-chip__meta {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 0.65rem;
  color: var(--text-muted);
  font-size: 0.88rem;
}

.wrong-problem-chip__label {
  color: var(--text-muted);
}

@media (max-width: 960px) {
  .my-info-layout {
    grid-template-columns: minmax(0, 1fr);
  }

  .recent-submission-item {
    gap: 0.65rem;
  }
}
</style>
