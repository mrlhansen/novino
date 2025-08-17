[BITS 64]

global acquire_lock:function (acquire_lock.end - acquire_lock)
acquire_lock:
    lock bts dword [rdi], 0  ; Attempt to acquire lock
    jc .spin
    ret
.spin:
    pause                    ; Tell CPU we're spinning
    test dword [rdi], 1      ; Is the lock free?
    jnz .spin                ; No, not yet
    jmp acquire_lock         ; Yes, retry
.end:

global release_lock:function (release_lock.end - release_lock)
release_lock:
    mov dword [rdi], 0
    ret
.end:

global acquire_safe_lock:function (acquire_safe_lock.end - acquire_safe_lock)
acquire_safe_lock:
    pushfw                   ; Push 16-bit flags register
    cli                      ; Disable interrupts
    pop word [rsi]           ; Save flags register
    jmp acquire_lock
.end:

global release_safe_lock:function (release_safe_lock.end - release_safe_lock)
release_safe_lock:
    mov dword [rdi], 0       ; Release lock
    push word [rsi]          ; Push 16-bit flags register
    popfw                    ; Restore flags register
    ret
.end:

global disable_interrupts:function (disable_interrupts.end - disable_interrupts)
disable_interrupts:
    pushfw                   ; Push 16-bit flags register
    cli                      ; Disable interrupts
    pop word [rdi]           ; Save flags register
    ret
.end:

global restore_interrupts:function (restore_interrupts.end - restore_interrupts)
restore_interrupts:
    push word [rdi]          ; Push 16-bit flags register
    popfw                    ; Restore flags register
    ret
.end:
