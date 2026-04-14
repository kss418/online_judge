<template>
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
          :value="section.model.descriptionDraft"
          class="admin-problem-textarea"
          spellcheck="false"
          @input="handleDescriptionInput"
        />
      </label>

      <label class="field-block">
        <span class="field-label">입력 설명</span>
        <textarea
          :value="section.model.inputFormatDraft"
          class="admin-problem-textarea"
          spellcheck="false"
          @input="handleInputFormatInput"
        />
      </label>

      <label class="field-block">
        <span class="field-label">출력 설명</span>
        <textarea
          :value="section.model.outputFormatDraft"
          class="admin-problem-textarea"
          spellcheck="false"
          @input="handleOutputFormatInput"
        />
      </label>

      <label class="field-block">
        <span class="field-label">비고</span>
        <textarea
          :value="section.model.noteDraft"
          class="admin-problem-textarea is-note"
          spellcheck="false"
          placeholder="없으면 비워 두세요."
          @input="handleNoteInput"
        />
      </label>
    </div>

    <div class="admin-problem-section-actions">
      <button
        type="button"
        class="primary-button"
        :disabled="!section.model.canSaveStatement"
        @click="handleSaveStatement"
      >
        {{ section.model.isSavingStatement ? '저장 중...' : '저장' }}
      </button>
    </div>
  </article>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  section: {
    type: Object,
    required: true
  }
})

const section = computed(() => props.section)

function handleDescriptionInput(event){
  section.value.actions.updateDescriptionDraft(event.target.value)
}

function handleInputFormatInput(event){
  section.value.actions.updateInputFormatDraft(event.target.value)
}

function handleOutputFormatInput(event){
  section.value.actions.updateOutputFormatDraft(event.target.value)
}

function handleNoteInput(event){
  section.value.actions.updateNoteDraft(event.target.value)
}

function handleSaveStatement(){
  section.value.actions.saveStatement()
}
</script>

<style scoped>
.admin-problem-editor-section {
  display: grid;
  gap: 1rem;
  padding: 1rem;
  border-radius: 22px;
  border: 1px solid var(--admin-problems-section-border);
  background: var(--admin-problems-section-surface);
  box-shadow: var(--admin-problems-section-shadow);
}

.admin-problem-form-grid {
  display: grid;
  gap: 0.9rem;
}

.single-column-grid {
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

.admin-problem-textarea:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.admin-problem-textarea.is-note {
  min-height: 7rem;
}

.admin-problem-section-actions {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

@media (max-width: 720px) {
  .admin-problem-section-actions {
    justify-content: stretch;
  }
}
</style>
