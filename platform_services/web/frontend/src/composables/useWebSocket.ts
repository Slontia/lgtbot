import { onUnmounted, ref } from 'vue'
import { useAuthStore } from '../stores/auth'

export type BotItem =
  | { type: 'text'; content: string }
  | { type: 'html'; content: string }
  | { type: 'image_url'; content: string; delete_after_send?: boolean }
  | { type: 'at'; content: string }

export type WsInbound =
  | { type: 'bot_message'; items: BotItem[] }
  | { type: 'commands_update'; commands: unknown[] }

export function useWebSocket(urlPath = '/ws') {
  const auth = useAuthStore()
  const connected = ref(false)
  const lastMessages = ref<string[]>([])
  let ws: WebSocket | null = null

  function connect() {
    if (!auth.token) return
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:'
    const qs = `?token=${encodeURIComponent(auth.token)}`
    ws = new WebSocket(`${proto}//${location.host}${urlPath}${qs}`)
    ws.onopen = () => {
      connected.value = true
    }
    ws.onclose = () => {
      connected.value = false
    }
    ws.onmessage = (ev) => {
      try {
        const data = JSON.parse(ev.data as string) as WsInbound
        if (data.type === 'bot_message') {
          const text = data.items
            .map((i) => ('content' in i ? String(i.content) : ''))
            .join(' ')
          lastMessages.value = [...lastMessages.value, text].slice(-50)
        }
      } catch {
        /* ignore */
      }
    }
  }

  function sendGameText(text: string) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return
    ws.send(JSON.stringify({ type: 'send_message', text }))
  }

  function disconnect() {
    ws?.close()
    ws = null
  }

  onUnmounted(disconnect)

  return { connected, lastMessages, connect, disconnect, sendGameText }
}
