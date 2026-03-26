<template>
  <Teleport to="body">
    <div
      v-if="open"
      class="auth-dialog-backdrop"
      @pointerdown="handleBackdropPointerDown"
      @click.self="handleBackdropClick"
    >
      <section
        class="auth-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="auth-dialog-title"
      >
        <div class="auth-dialog-header">
          <div>
            <p class="panel-kicker">account</p>
            <h3 id="auth-dialog-title">{{ dialogTitle }}</h3>
          </div>
          <button
            type="button"
            class="icon-button"
            :disabled="authState.isSubmitting"
            aria-label="닫기"
            @click="handleClose"
          >
            ×
          </button>
        </div>

        <form
          v-if="activeMode === 'login'"
          class="auth-form"
          @submit.prevent="submitLogin"
        >
          <label class="field-block">
            <span class="field-label">ID</span>
            <input
              v-model.trim="loginForm.user_login_id"
              class="field-input"
              type="text"
              autocomplete="username"
              required
            />
          </label>

          <label class="field-block">
            <span class="field-label">비밀번호</span>
            <input
              v-model="loginForm.raw_password"
              class="field-input"
              type="password"
              autocomplete="current-password"
              required
            />
          </label>

          <p v-if="submitErrorMessage" class="form-error">
            {{ submitErrorMessage }}
          </p>

          <div class="dialog-actions">
            <button
              type="button"
              class="ghost-button"
              :disabled="authState.isSubmitting"
              @click="setMode('sign-up')"
            >
              회원가입으로 이동
            </button>
            <button
              type="submit"
              class="primary-button"
              :disabled="authState.isSubmitting"
            >
              {{ authState.isSubmitting ? '로그인 중...' : '로그인' }}
            </button>
          </div>
        </form>

        <form
          v-else
          class="auth-form"
          @submit.prevent="submitSignUp"
        >
          <div class="field-grid">
            <label class="field-block">
              <span class="field-label">ID</span>
              <input
                v-model.trim="signUpForm.user_login_id"
                class="field-input"
                type="text"
                autocomplete="username"
                required
              />
            </label>

            <label class="field-block">
              <span class="field-label">닉네임</span>
              <input
                v-model.trim="signUpForm.user_name"
                class="field-input"
                type="text"
                autocomplete="nickname"
                required
              />
            </label>
          </div>

          <label class="field-block">
            <span class="field-label">비밀번호</span>
            <input
              v-model="signUpForm.raw_password"
              class="field-input"
              type="password"
              autocomplete="new-password"
              required
            />
          </label>

          <p class="form-hint">
            회원가입이 성공하면 바로 로그인된 상태로 전환됩니다.
          </p>

          <p v-if="submitErrorMessage" class="form-error">
            {{ submitErrorMessage }}
          </p>

          <div class="dialog-actions">
            <button
              type="button"
              class="ghost-button"
              :disabled="authState.isSubmitting"
              @click="setMode('login')"
            >
              로그인으로 이동
            </button>
            <button
              type="submit"
              class="primary-button"
              :disabled="authState.isSubmitting"
            >
              {{ authState.isSubmitting ? '가입 중...' : '회원가입' }}
            </button>
          </div>
        </form>
      </section>
    </div>
  </Teleport>
</template>

<script setup>
import { computed, reactive, ref, watch } from 'vue'

import { useAuth } from '@/composables/useAuth'

const props = defineProps({
  open: {
    type: Boolean,
    default: false
  },
  initialMode: {
    type: String,
    default: 'login'
  }
})

const emit = defineEmits(['close'])

const { authState, login, signUp } = useAuth()

const activeMode = ref('login')
const submitErrorMessage = ref('')
const isBackdropInteraction = ref(false)

const loginForm = reactive({
  user_login_id: '',
  raw_password: ''
})

const signUpForm = reactive({
  user_login_id: '',
  user_name: '',
  raw_password: ''
})

const dialogTitle = computed(() =>
  activeMode.value === 'login' ? '계정 로그인' : '새 계정 만들기'
)

watch(
  () => props.initialMode,
  (mode) => {
    activeMode.value = mode === 'sign-up' ? 'sign-up' : 'login'
    submitErrorMessage.value = ''
  },
  { immediate: true }
)

watch(
  () => props.open,
  (isOpen) => {
    if (!isOpen) {
      isBackdropInteraction.value = false
    }

    if (isOpen) {
      submitErrorMessage.value = ''
    }
  }
)

function setMode(mode){
  activeMode.value = mode
  submitErrorMessage.value = ''
}

function handleClose(){
  if (authState.isSubmitting) {
    return
  }

  isBackdropInteraction.value = false
  emit('close')
}

function handleBackdropPointerDown(event){
  isBackdropInteraction.value = event.target === event.currentTarget
}

function handleBackdropClick(){
  if (!isBackdropInteraction.value) {
    return
  }

  handleClose()
}

function clearForms(){
  loginForm.user_login_id = ''
  loginForm.raw_password = ''
  signUpForm.user_login_id = ''
  signUpForm.user_name = ''
  signUpForm.raw_password = ''
}

async function submitLogin(){
  submitErrorMessage.value = ''

  try {
    await login({
      user_login_id: loginForm.user_login_id,
      raw_password: loginForm.raw_password
    })

    clearForms()
    emit('close')
  } catch (error) {
    submitErrorMessage.value = error instanceof Error
      ? error.message
      : '로그인에 실패했습니다.'
  }
}

async function submitSignUp(){
  submitErrorMessage.value = ''

  try {
    await signUp({
      user_login_id: signUpForm.user_login_id,
      user_name: signUpForm.user_name,
      raw_password: signUpForm.raw_password
    })

    clearForms()
    emit('close')
  } catch (error) {
    submitErrorMessage.value = error instanceof Error
      ? error.message
      : '회원가입에 실패했습니다.'
  }
}
</script>
