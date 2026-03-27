<template>
  <section class="admin-problem-editor-panel">
    <div v-if="isLoadingDetail" class="empty-state">
      <p>문제 정보를 불러오는 중입니다.</p>
    </div>

    <div v-else-if="detailErrorMessage" class="empty-state error-state">
      <p>{{ detailErrorMessage }}</p>
    </div>

    <div v-else-if="!selectedProblemDetail" class="empty-state">
      <p>왼쪽 목록에서 문제를 선택하거나 새 문제를 생성하세요.</p>
    </div>

    <template v-else>
      <div class="admin-problem-editor-header">
        <div>
          <p class="panel-kicker">editor</p>
          <h3 class="admin-problem-heading">
            <span class="admin-problem-heading-id-group">
              <span class="admin-problem-heading-id">
                #{{ formatCount(selectedProblemDetail.problem_id) }}
              </span>
              <span class="admin-problem-heading-divider" aria-hidden="true">·</span>
            </span>
            <span class="admin-problem-heading-text">{{ selectedProblemDetail.title }}</span>
          </h3>
          <p class="admin-problem-editor-copy">
            선택한 문제의 제목, 제한, 설명, 공개 예제를 바로 저장할 수 있습니다.
          </p>
        </div>

        <div class="admin-problem-editor-header-actions">
          <button
            type="button"
            class="ghost-button admin-problem-rejudge-button"
            :disabled="Boolean(busySection)"
            @click="$emit('open-rejudge-dialog')"
          >
            재채점
          </button>
          <RouterLink
            class="ghost-button"
            :to="{ name: 'admin-problem-testcases', params: { problemId: selectedProblemDetail.problem_id } }"
          >
            테스트케이스 관리
          </RouterLink>
          <RouterLink
            class="ghost-button"
            :to="{ name: 'problem-detail', params: { problemId: selectedProblemDetail.problem_id } }"
          >
            문제 보기
          </RouterLink>
        </div>
      </div>

      <article class="admin-problem-editor-section admin-problem-editor-section--title">
        <div class="panel-header">
          <div>
            <p class="panel-kicker">title</p>
            <h3>제목</h3>
          </div>
        </div>

        <div class="admin-problem-form-grid single-column-grid">
          <input
            :value="titleDraft"
            class="field-input"
            type="text"
            maxlength="120"
            placeholder="문제 제목"
            @input="$emit('update:titleDraft', $event.target.value)"
          />
        </div>

        <div class="admin-problem-section-actions">
          <button
            type="button"
            class="primary-button"
            :disabled="!canSaveTitle"
            @click="$emit('save-title')"
          >
            {{ isSavingTitle ? '저장 중...' : '저장' }}
          </button>
        </div>
      </article>

      <article class="admin-problem-editor-section admin-problem-editor-section--limits">
        <div class="panel-header">
          <div>
            <p class="panel-kicker">limits</p>
            <h3>실행 제한</h3>
          </div>
        </div>

        <div class="admin-problem-form-grid">
          <label class="field-block">
            <span class="field-label">시간 제한(ms)</span>
            <input
              :value="timeLimitDraft"
              class="field-input"
              type="number"
              min="1"
              inputmode="numeric"
              placeholder="1000"
              @input="$emit('update:timeLimitDraft', $event.target.value)"
            />
          </label>

          <label class="field-block">
            <span class="field-label">메모리 제한(MB)</span>
            <input
              :value="memoryLimitDraft"
              class="field-input"
              type="number"
              min="1"
              inputmode="numeric"
              placeholder="256"
              @input="$emit('update:memoryLimitDraft', $event.target.value)"
            />
          </label>
        </div>

        <div class="admin-problem-section-actions">
          <button
            type="button"
            class="primary-button"
            :disabled="!canSaveLimits"
            @click="$emit('save-limits')"
          >
            {{ isSavingLimits ? '저장 중...' : '저장' }}
          </button>
        </div>
      </article>

      <article class="admin-problem-editor-section admin-problem-editor-section--statement">
        <div class="panel-header">
          <div>
            <p class="panel-kicker">statement</p>
            <h3>문제 설명</h3>
          </div>
        </div>

        <div class="admin-problem-form-grid single-column-grid">
          <label class="field-block">
            <span class="field-label">문제 설명</span>
            <textarea
              :value="descriptionDraft"
              class="admin-problem-textarea"
              spellcheck="false"
              @input="$emit('update:descriptionDraft', $event.target.value)"
            />
          </label>

          <label class="field-block">
            <span class="field-label">입력 설명</span>
            <textarea
              :value="inputFormatDraft"
              class="admin-problem-textarea"
              spellcheck="false"
              @input="$emit('update:inputFormatDraft', $event.target.value)"
            />
          </label>

          <label class="field-block">
            <span class="field-label">출력 설명</span>
            <textarea
              :value="outputFormatDraft"
              class="admin-problem-textarea"
              spellcheck="false"
              @input="$emit('update:outputFormatDraft', $event.target.value)"
            />
          </label>

          <label class="field-block">
            <span class="field-label">비고</span>
            <textarea
              :value="noteDraft"
              class="admin-problem-textarea is-note"
              spellcheck="false"
              placeholder="없으면 비워 두세요."
              @input="$emit('update:noteDraft', $event.target.value)"
            />
          </label>
        </div>

        <div class="admin-problem-section-actions">
          <button
            type="button"
            class="primary-button"
            :disabled="!canSaveStatement"
            @click="$emit('save-statement')"
          >
            {{ isSavingStatement ? '저장 중...' : '저장' }}
          </button>
        </div>
      </article>

      <article class="admin-problem-editor-section admin-problem-editor-section--samples">
        <div class="panel-header admin-sample-section-header">
          <div>
            <p class="panel-kicker">samples</p>
            <h3>공개 예제</h3>
            <p class="admin-problem-section-copy">
              문제 상세 페이지에 노출되는 공개 예제를 행별로 관리합니다. 추가 시 빈 샘플이 생성되며, 삭제 버튼은 마지막 샘플에만 표시됩니다.
            </p>
          </div>

          <button
            type="button"
            class="ghost-button"
            :disabled="!canCreateSample"
            @click="$emit('create-sample')"
          >
            {{ isCreatingSample ? '추가 중...' : '샘플 추가' }}
          </button>
        </div>

        <div v-if="!sampleDrafts.length" class="empty-state compact-state">
          <p>등록된 공개 예제가 아직 없습니다.</p>
        </div>

        <div v-else class="admin-sample-list">
          <article
            v-for="sampleDraft in sampleDrafts"
            :key="sampleDraft.sample_order"
            class="admin-sample-card"
          >
            <div class="admin-sample-card-header">
              <div>
                <p class="panel-kicker">sample {{ formatCount(sampleDraft.sample_order) }}</p>
                <h3>예제 {{ formatCount(sampleDraft.sample_order) }}</h3>
              </div>

              <button
                v-if="isLastSample(sampleDraft.sample_order)"
                type="button"
                class="ghost-button admin-sample-delete-button"
                :disabled="!canDeleteLastSample"
                @click="$emit('delete-last-sample')"
              >
                {{ isDeletingLastSample ? '삭제 중...' : '샘플 삭제' }}
              </button>
            </div>

            <div class="admin-problem-form-grid">
              <label class="field-block">
                <span class="field-label">입력</span>
                <textarea
                  v-model="sampleDraft.sample_input"
                  class="admin-problem-textarea admin-sample-textarea"
                  spellcheck="false"
                  placeholder="빈 입력도 허용됩니다."
                />
              </label>

              <label class="field-block">
                <span class="field-label">출력</span>
                <textarea
                  v-model="sampleDraft.sample_output"
                  class="admin-problem-textarea admin-sample-textarea"
                  spellcheck="false"
                  placeholder="빈 출력도 허용됩니다."
                />
              </label>
            </div>

            <div class="admin-problem-section-actions">
              <button
                type="button"
                class="primary-button"
                :disabled="!canSaveSample(sampleDraft.sample_order)"
                @click="$emit('save-sample', sampleDraft.sample_order)"
              >
                {{ isSavingSample(sampleDraft.sample_order) ? '저장 중...' : '저장' }}
              </button>
            </div>
          </article>
        </div>
      </article>

      <article class="admin-problem-editor-section admin-problem-editor-section--testcases">
        <div class="panel-header admin-sample-section-header">
          <div>
            <p class="panel-kicker">testcases</p>
            <h3>테스트케이스</h3>
            <p class="admin-problem-section-copy">
              ZIP 업로드 시 기존 테스트케이스는 모두 교체됩니다. `001.in` / `001.out`부터
              `999.in` / `999.out`까지 연속 순번만 허용하며, 빈 파일은 허용됩니다.
            </p>
          </div>
        </div>

        <div class="admin-testcase-upload-panel">
          <label class="field-block">
            <span class="field-label">ZIP 파일</span>
            <input
              :key="testcaseZipInputKey"
              class="admin-testcase-file-input"
              type="file"
              accept=".zip,application/zip"
              :disabled="Boolean(busySection)"
              @change="$emit('testcase-zip-change', $event)"
            />
          </label>

          <p v-if="selectedTestcaseZipName" class="admin-testcase-selected-file">
            선택한 파일: {{ selectedTestcaseZipName }}
          </p>
        </div>

        <div class="admin-problem-section-actions">
          <button
            type="button"
            class="primary-button"
            :disabled="!canUploadTestcaseZip"
            @click="$emit('upload-testcase-zip')"
          >
            {{ isUploadingTestcaseZip ? '업로드 중...' : 'ZIP 업로드' }}
          </button>
        </div>
      </article>

      <article class="admin-problem-editor-section is-danger">
        <div class="panel-header">
          <div>
            <p class="panel-kicker">danger zone</p>
            <h3>문제 삭제</h3>
          </div>
        </div>

        <p class="admin-problem-danger-copy">
          삭제 전에 문제 번호와 제목을 다시 입력해야 합니다. 제출이 연결된 문제는 삭제가 거부될 수 있습니다.
        </p>

        <div class="admin-problem-section-actions">
          <button
            type="button"
            class="ghost-button admin-problem-danger-button"
            :disabled="Boolean(busySection)"
            @click="$emit('open-delete-dialog')"
          >
            문제 삭제
          </button>
        </div>
      </article>
    </template>
  </section>
</template>

<script setup>
defineProps({
  isLoadingDetail: {
    type: Boolean,
    required: true
  },
  detailErrorMessage: {
    type: String,
    required: true
  },
  selectedProblemDetail: {
    type: Object,
    default: null
  },
  titleDraft: {
    type: String,
    default: ''
  },
  timeLimitDraft: {
    type: String,
    default: ''
  },
  memoryLimitDraft: {
    type: String,
    default: ''
  },
  descriptionDraft: {
    type: String,
    default: ''
  },
  inputFormatDraft: {
    type: String,
    default: ''
  },
  outputFormatDraft: {
    type: String,
    default: ''
  },
  noteDraft: {
    type: String,
    default: ''
  },
  sampleDrafts: {
    type: Array,
    required: true
  },
  testcaseZipInputKey: {
    type: Number,
    required: true
  },
  selectedTestcaseZipName: {
    type: String,
    default: ''
  },
  busySection: {
    type: String,
    default: ''
  },
  canSaveTitle: {
    type: Boolean,
    required: true
  },
  canSaveLimits: {
    type: Boolean,
    required: true
  },
  canSaveStatement: {
    type: Boolean,
    required: true
  },
  canCreateSample: {
    type: Boolean,
    required: true
  },
  canUploadTestcaseZip: {
    type: Boolean,
    required: true
  },
  canDeleteLastSample: {
    type: Boolean,
    required: true
  },
  isSavingTitle: {
    type: Boolean,
    required: true
  },
  isSavingLimits: {
    type: Boolean,
    required: true
  },
  isSavingStatement: {
    type: Boolean,
    required: true
  },
  isCreatingSample: {
    type: Boolean,
    required: true
  },
  isDeletingLastSample: {
    type: Boolean,
    required: true
  },
  isUploadingTestcaseZip: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  },
  isSavingSample: {
    type: Function,
    required: true
  },
  canSaveSample: {
    type: Function,
    required: true
  },
  isLastSample: {
    type: Function,
    required: true
  }
})

defineEmits([
  'update:titleDraft',
  'update:timeLimitDraft',
  'update:memoryLimitDraft',
  'update:descriptionDraft',
  'update:inputFormatDraft',
  'update:outputFormatDraft',
  'update:noteDraft',
  'save-title',
  'save-limits',
  'save-statement',
  'create-sample',
  'save-sample',
  'delete-last-sample',
  'testcase-zip-change',
  'upload-testcase-zip',
  'open-rejudge-dialog',
  'open-delete-dialog'
])
</script>

<style scoped>
.admin-problem-editor-panel {
  display: grid;
  gap: 1rem;
  min-height: 40rem;
  padding: 1rem;
  border: 1px solid var(--admin-problems-shell-border);
  border-radius: 24px;
  background: var(--admin-problems-shell-surface);
  box-shadow: var(--admin-problems-shell-shadow);
}

.admin-problem-editor-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-heading {
  display: flex;
  align-items: baseline;
  gap: 0.75rem;
  flex-wrap: wrap;
  min-width: 0;
  margin: 0;
}

.admin-problem-heading-id-group {
  display: inline-flex;
  align-items: baseline;
  gap: 0.5rem;
  white-space: nowrap;
  color: var(--ink-soft);
  flex-shrink: 0;
}

.admin-problem-heading-id {
  font-weight: 800;
  letter-spacing: -0.03em;
}

.admin-problem-heading-divider {
  color: rgba(20, 33, 61, 0.34);
  font-weight: 700;
}

.admin-problem-heading-text {
  min-width: 0;
  color: var(--ink-strong);
}

.admin-problem-editor-copy,
.admin-problem-section-copy,
.admin-problem-danger-copy {
  margin: 0;
  color: var(--ink-soft);
}

.admin-problem-editor-header-actions {
  display: flex;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-editor-section {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 22px;
  border: 1px solid var(--admin-problems-section-border);
  background: var(--admin-problems-section-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-problem-editor-section.is-danger {
  border-color: rgba(185, 28, 28, 0.16);
  background: linear-gradient(
    180deg,
    rgba(255, 250, 250, 0.98),
    rgba(254, 242, 242, 0.94)
  );
}

.admin-sample-section-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-problem-form-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 0.9rem;
}

.admin-problem-form-grid.single-column-grid {
  grid-template-columns: minmax(0, 1fr);
}

.admin-problem-textarea {
  width: 100%;
  min-height: 10rem;
  padding: 1rem;
  resize: vertical;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: var(--admin-problems-nested-surface);
  color: var(--ink-strong);
  font: inherit;
  line-height: 1.6;
}

.admin-problem-textarea.is-note {
  min-height: 7rem;
}

.admin-sample-list {
  display: grid;
  gap: 1rem;
}

.admin-sample-card {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 20px;
  border: 1px solid var(--admin-problems-nested-border);
  background: var(--admin-problems-nested-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-sample-card-header {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.admin-sample-textarea {
  min-height: 9rem;
  font-family: "SFMono-Regular", "Consolas", monospace;
}

.admin-sample-delete-button {
  color: var(--danger);
  background: rgba(254, 242, 242, 0.96);
  border-color: rgba(185, 28, 28, 0.18);
}

.admin-testcase-upload-panel {
  display: grid;
  gap: 0.75rem;
  padding: 0.95rem 1rem;
  border: 1px dashed rgba(148, 163, 184, 0.24);
  border-radius: 18px;
  background: rgba(255, 255, 255, 0.72);
}

.admin-testcase-file-input {
  display: block;
  width: 100%;
  padding: 0.85rem 1rem;
  border-radius: 18px;
  border: 1px dashed rgba(20, 33, 61, 0.18);
  background: var(--admin-problems-nested-surface);
  color: var(--ink-strong);
  font: inherit;
}

.admin-testcase-selected-file {
  margin: 0;
  color: var(--ink-soft);
  word-break: break-all;
}

.admin-problem-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-problem-section-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.admin-problem-rejudge-button,
.admin-problem-danger-button {
  min-width: 8.5rem;
}

.admin-problem-rejudge-button {
  min-width: 6.25rem;
  padding-inline: 0.95rem;
  color: var(--warning);
  background: rgba(255, 247, 237, 0.96);
  border-color: rgba(180, 83, 9, 0.18);
}

.admin-problem-danger-button {
  color: var(--danger);
  background: rgba(254, 242, 242, 0.96);
  border-color: rgba(185, 28, 28, 0.18);
}

@media (max-width: 1100px) {
  .admin-problem-editor-header,
  .admin-sample-section-header,
  .admin-sample-card-header {
    flex-direction: column;
  }
}

@media (max-width: 720px) {
  .admin-problem-form-grid {
    grid-template-columns: minmax(0, 1fr);
  }

  .admin-problem-section-actions {
    justify-content: stretch;
  }
}
</style>
