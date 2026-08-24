.data
float_0_75 REAL4 0.75
float_0_6  REAL4 0.6
float_1_1  REAL4 1.1
float_1_0  REAL4 1.0
float_0_1  REAL4 0.1

.code

; =============================================================================
; MASM x64 Trampolines and Mid-function Hooks for CubeForgeLoader
; Built with Microsoft Macro Assembler (ml64.exe)
; =============================================================================

; Helper Macros
PUSH_ALL MACRO
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
ENDM

POP_ALL MACRO
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
ENDM

PREPARE_STACK MACRO
    mov rax, rsp
    and rsp, -16
    push rax
    sub rsp, 28h
ENDM

RESTORE_STACK MACRO
    add rsp, 28h
    pop rsp
ENDM

; =============================================================================
; External Declarations (C++ Functions and Jump Target Pointers)
; =============================================================================

EXTERN StartMods:PROC
EXTERN initterm_e:QWORD

EXTERN ChatHandler:PROC
EXTERN ASMChatHandler_jmpback:QWORD
EXTERN ASMChatHandler_bail:QWORD

EXTERN CheckInventoryFullHandler:PROC
EXTERN ASMCheckInventoryFullHandler_jmpback:QWORD
EXTERN ASMCheckInventoryFullHandler_retn:QWORD

EXTERN GameTickHandler:PROC
EXTERN ASMGameTickHandler_jmpback:QWORD

EXTERN P2PRequestHandler:PROC
EXTERN ASMP2PRequestHandler_jmpback:QWORD
EXTERN ASMP2PRequestHandler_block:QWORD
EXTERN ASMP2PRequestHandler_allow:QWORD

EXTERN PresentHandler:PROC
EXTERN ASM_PresentHandler_jmpback:QWORD

EXTERN WindowProcHandler:PROC
EXTERN ASM_WindowProcHandler_jmpback:QWORD

EXTERN GetKeyboardStateHandler:PROC
EXTERN ASM_GetKeyboardStateHandler_jmpback:QWORD

EXTERN GetMouseStateHandler:PROC
EXTERN ASM_GetMouseStateHandler_jmpback:QWORD

EXTERN ZoneGeneratedHandler:PROC
EXTERN ASM_ZoneGeneratedHandler_jmpback:QWORD

EXTERN ZoneDestroyHandler:PROC
EXTERN ASM_ZoneDestroyHandler_jmpback:QWORD

EXTERN ChunkRemeshHandler:PROC
EXTERN ASM_ChunkRemeshHandler_jmpback:QWORD

EXTERN ChunkRemeshedHandler:PROC
EXTERN ASM_ChunkRemeshedHandler_jmpback:QWORD

EXTERN CreatureAttackPowerCalculatedHandler:PROC
EXTERN CreatureCriticalCalculatedHandler:PROC
EXTERN CreatureHasteCalculatedHandler:PROC
EXTERN CreatureHPCalculatedHandler:PROC
EXTERN CreatureManaGenerationCalculatedHandler:PROC
EXTERN CreatureRegenerationCalculatedHandler:PROC
EXTERN CreatureResistanceCalculatedHandler:PROC
EXTERN CreatureSpellPowerCalculatedHandler:PROC

EXTERN cube__Creature__OnCreatureDeath:PROC
EXTERN ASM_cube__Creature__OnCreatureDeath_JMPBACK:QWORD

EXTERN cube__Creature__OnPlayerCombatDeath:PROC
EXTERN SoundPacket__ctor:PROC
EXTERN ASM_cube__Creature__OnPlayerCombatDeath_JMPBACK:QWORD

EXTERN cube__Creature__OnPlayerDrownDeath:PROC
EXTERN ASM_cube__Creature__OnPlayerDrownDeath_JMPBACK:QWORD

EXTERN cube__Creature__OnPlayerFallDeath:PROC
EXTERN ASM_cube__Creature__OnPlayerFallDeath_JMPBACK:QWORD

EXTERN cube__Game__MouseUp:PROC
EXTERN ASM_cube__Game__MouseUp_jmpback:QWORD

EXTERN cube__Game__Update:PROC
EXTERN ASM_cube__Game__Update_jmpback:QWORD
EXTERN ASM_cube__Game__Update_bail:QWORD

EXTERN cube__Item__GetBuyingPrice:PROC

; =============================================================================
; Trampolines Implementation
; =============================================================================

ASMStartMods PROC
    PUSH_ALL
    PREPARE_STACK
    call StartMods
    RESTORE_STACK
    POP_ALL
    jmp qword ptr [initterm_e]
ASMStartMods ENDP

ASMChatHandler PROC
    PUSH_ALL
    mov rcx, rbx
    PREPARE_STACK
    call ChatHandler
    RESTORE_STACK
    test al, al
    jnz chat_bail
    POP_ALL
    mov [rsp+48h], rax
    mov r8, [rbx+10h]
    cmp qword ptr [rbx+18h], 8
    jb chat_lbl1
    mov rbx, [rbx]
chat_lbl1:
    jmp qword ptr [ASMChatHandler_jmpback]
chat_bail:
    POP_ALL
    jmp qword ptr [ASMChatHandler_bail]
ASMChatHandler ENDP

ASMCheckInventoryFullHandler PROC
    PUSH_ALL
    PREPARE_STACK
    call CheckInventoryFullHandler
    RESTORE_STACK
    cmp eax, 1
    je inv_full
    cmp eax, 2
    je inv_not_full
    POP_ALL
    mov [rsp+20h], rbp
    push r12
    push r14
    push r15
    sub rsp, 20h
    jmp qword ptr [ASMCheckInventoryFullHandler_jmpback]
inv_full:
    POP_ALL
    xor al, al
    jmp qword ptr [ASMCheckInventoryFullHandler_retn]
inv_not_full:
    POP_ALL
    mov al, 1
    jmp qword ptr [ASMCheckInventoryFullHandler_retn]
ASMCheckInventoryFullHandler ENDP

ASMGameTickHandler PROC
    PUSH_ALL
    mov rcx, rax
    PREPARE_STACK
    call GameTickHandler
    RESTORE_STACK
    POP_ALL
    mov [rsp+20h], ebx
    xor r9d, r9d
    xor r8d, r8d
    xor edx, edx
    lea rcx, [rbp+0B8h]
    jmp qword ptr [ASMGameTickHandler_jmpback]
ASMGameTickHandler ENDP

ASMP2PRequestHandler PROC
    PUSH_ALL
    mov rcx, [rdi]
    PREPARE_STACK
    call P2PRequestHandler
    RESTORE_STACK
    cmp eax, 1
    je p2p_block
    cmp eax, 2
    je p2p_allow
    POP_ALL
    mov edx, 4
    mov rcx, [rax]
    mov rax, [rcx]
    call qword ptr [rax+18h]
    jmp qword ptr [ASMP2PRequestHandler_jmpback]
p2p_block:
    POP_ALL
    jmp qword ptr [ASMP2PRequestHandler_block]
p2p_allow:
    POP_ALL
    jmp qword ptr [ASMP2PRequestHandler_allow]
ASMP2PRequestHandler ENDP

ASM_PresentHandler PROC
    setnz bl
    xor r8d, r8d
    mov rax, [rcx]
    mov edx, ebx
    PUSH_ALL
    PREPARE_STACK
    call PresentHandler
    RESTORE_STACK
    POP_ALL
    call qword ptr [rax+40h]
    jmp qword ptr [ASM_PresentHandler_jmpback]
ASM_PresentHandler ENDP

ASM_WindowProcHandler PROC
    PUSH_ALL
    PREPARE_STACK
    call WindowProcHandler
    RESTORE_STACK
    cmp eax, 1
    je wndproc_block
    POP_ALL
    mov [rsp+8], rbx
    push rdi
    sub rsp, 20h
    mov rbx, r8
    mov r10, rcx
    jmp qword ptr [ASM_WindowProcHandler_jmpback]
wndproc_block:
    POP_ALL
    xor eax, eax
    ret
ASM_WindowProcHandler ENDP

ASM_GetKeyboardStateHandler PROC
    mov rcx, qword ptr [rbp-50h]
    mov rax, [rcx]
    lea r8, [rbp+480h]
    mov edx, r14d
    call qword ptr [rax+48h]
    PUSH_ALL
    lea rcx, [rbp+480h]
    PREPARE_STACK
    call GetKeyboardStateHandler
    RESTORE_STACK
    POP_ALL
    jmp qword ptr [ASM_GetKeyboardStateHandler_jmpback]
ASM_GetKeyboardStateHandler ENDP

ASM_GetMouseStateHandler PROC
    mov rcx, qword ptr [rbp-78h]
    mov rax, [rcx]
    lea r8, [rbp+1B0h]
    mov edx, 10h
    call qword ptr [rax+48h]
    PUSH_ALL
    lea rcx, [rbp+1B0h]
    PREPARE_STACK
    call GetMouseStateHandler
    RESTORE_STACK
    POP_ALL
    jmp qword ptr [ASM_GetMouseStateHandler_jmpback]
ASM_GetMouseStateHandler ENDP

ASM_ZoneGeneratedHandler PROC
    PUSH_ALL
    mov rcx, rbp
    PREPARE_STACK
    call ZoneGeneratedHandler
    RESTORE_STACK
    POP_ALL
    mov rax, [rsp+48h]
    mov [rax+18h], rbp
    lea rcx, [r14+3C8h]
    jmp qword ptr [ASM_ZoneGeneratedHandler_jmpback]
ASM_ZoneGeneratedHandler ENDP

ASM_ZoneDestroyHandler PROC
    PUSH_ALL
    PREPARE_STACK
    call ZoneDestroyHandler
    RESTORE_STACK
    POP_ALL
    push rsi
    push rdi
    push r14
    sub rsp, 30h
    mov qword ptr [rsp+20h], 0FFFFFFFFFFFFFFFEh
    jmp qword ptr [ASM_ZoneDestroyHandler_jmpback]
ASM_ZoneDestroyHandler ENDP

ASM_ChunkRemeshHandler PROC
    PUSH_ALL
    PREPARE_STACK
    mov rcx, r13
    call ChunkRemeshHandler
    RESTORE_STACK
    POP_ALL
    lea ebx, [r14+40h]
    mov [rbp-54h], ebx
    lea edi, [r15+40h]
    mov [rbp-64h], edi
    jmp qword ptr [ASM_ChunkRemeshHandler_jmpback]
ASM_ChunkRemeshHandler ENDP

ASM_ChunkRemeshedHandler PROC
    PUSH_ALL
    PREPARE_STACK
    mov rcx, r8
    call ChunkRemeshedHandler
    RESTORE_STACK
    POP_ALL
    mov rdx, [rdi]
    mov [rdi], rdi
    mov rax, [rbp+20h]
    mov [rax+8], rax
    jmp qword ptr [ASM_ChunkRemeshedHandler_jmpback]
ASM_ChunkRemeshedHandler ENDP

ASM_CreatureAttackPowerCalculatedHandler PROC
    PUSH_ALL
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    mov rcx, rsi
    PREPARE_STACK
    call CreatureAttackPowerCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    movaps xmm6, [rsp+70h]
    movaps xmm7, [rsp+60h]
    mov rsp, r11
    pop r14
    pop rdi
    pop rsi
    ret
ASM_CreatureAttackPowerCalculatedHandler ENDP

ASM_CreatureCriticalCalculatedHandler PROC
    movaps xmm0, xmm6
    PUSH_ALL
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    mov rcx, rbx
    PREPARE_STACK
    call CreatureCriticalCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    mov rbx, [rsp+40h]
    movaps xmm6, [rsp+20h]
    mov rsi, [rsp+48h]
    add rsp, 30h
    pop rdi
    ret
ASM_CreatureCriticalCalculatedHandler ENDP

ASM_CreatureHasteCalculatedHandler PROC
    mov [rsp-80h], rbx
    mov rbx, [rsp+50h]
    shr eax, 9
    test al, 1
    jz haste_lbl0
    mulss xmm7, float_0_75
    jmp haste_lbl0
haste_lbl0:
    mov eax, ecx
    shr eax, 12h
    test al, 1
    jz haste_lbl1
    mulss xmm7, float_0_6
    jmp haste_lbl1
haste_lbl1:
    bt ecx, 1Ah
    jnb haste_lbl2
    mulss xmm7, float_1_1
    jmp haste_lbl2
haste_lbl2:
    movaps xmm0, xmm7
    PUSH_ALL
    mov rcx, [rsp-8h]
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    PREPARE_STACK
    call CreatureHasteCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    movaps xmm7, [rsp+30h]
    add rsp, 58h
    ret
ASM_CreatureHasteCalculatedHandler ENDP

ASM_CreatureHPCalculatedHandler PROC
    movaps xmm0, xmm6
    PUSH_ALL
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    mov rcx, rbx
    PREPARE_STACK
    call CreatureHPCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    movaps xmm6, [rsp+20h]
    add rsp, 30h
    pop rbx
    ret
ASM_CreatureHPCalculatedHandler ENDP

ASM_CreatureManaGenerationCalculatedHandler PROC
    push rcx
    mov rcx, [rcx+980h]
    mov rax, [rcx]
    cmp rax, rcx
    jz mana_lbl2
mana_lbl1:
    cmp byte ptr [rax+10h], 22h
    jz mana_lbl3
    mov rax, [rax]
    cmp rax, rcx
    jnz mana_lbl1
mana_lbl2:
    movss xmm0, float_1_0
    jmp mana_lbl7
mana_lbl3:
    add rax, 10h
    jz mana_lbl2
    movss xmm1, dword ptr [rax+4]
    mulss xmm1, float_0_1
    movss xmm0, float_1_0
    subss xmm0, xmm1
mana_lbl7:
    PUSH_ALL
    mov rcx, [rsp+78h]
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    PREPARE_STACK
    call CreatureManaGenerationCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    add rsp, 8
    ret
ASM_CreatureManaGenerationCalculatedHandler ENDP

ASM_CreatureRegenerationCalculatedHandler PROC
    movaps xmm0, xmm6
    PUSH_ALL
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    mov rcx, rbx
    PREPARE_STACK
    call CreatureRegenerationCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    movaps xmm6, [rsp+20h]
    add rsp, 30h
    pop rbx
    ret
ASM_CreatureRegenerationCalculatedHandler ENDP

ASM_CreatureResistanceCalculatedHandler PROC
    movaps xmm0, xmm6
    PUSH_ALL
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    mov rcx, rbx
    PREPARE_STACK
    call CreatureResistanceCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    mov rbx, [rsp+40h]
    movaps xmm6, [rsp+20h]
    add rsp, 30h
    pop rdi
    ret
ASM_CreatureResistanceCalculatedHandler ENDP

ASM_CreatureSpellPowerCalculatedHandler PROC
    PUSH_ALL
    movq rax, xmm0
    push rax
    lea rdx, [rsp]
    mov rcx, rsi
    PREPARE_STACK
    call CreatureSpellPowerCalculatedHandler
    RESTORE_STACK
    pop rax
    movq xmm0, rax
    POP_ALL
    mov rbp, [r11+30h]
    movaps xmm6, [rsp+60h]
    mov rsp, r11
    pop r14
    pop rdi
    pop rsi
    ret
ASM_CreatureSpellPowerCalculatedHandler ENDP

ASM_cube__Creature__OnCreatureDeath PROC
    mov rdx, r15
    mov rcx, r13
    call cube__Creature__OnCreatureDeath
    xor r15d, r15d
    mov dword ptr [rbp-41h], 3F800000h
    jmp qword ptr [ASM_cube__Creature__OnCreatureDeath_JMPBACK]
ASM_cube__Creature__OnCreatureDeath ENDP

ASM_cube__Creature__OnPlayerCombatDeath PROC
    mov rcx, r13
    call cube__Creature__OnPlayerCombatDeath
    lea rcx, [rbp+0A20h]
    call SoundPacket__ctor
    jmp qword ptr [ASM_cube__Creature__OnPlayerCombatDeath_JMPBACK]
ASM_cube__Creature__OnPlayerCombatDeath ENDP

ASM_cube__Creature__OnPlayerDrownDeath PROC
    mov rcx, r13
    call cube__Creature__OnPlayerDrownDeath
    mov qword ptr [rbp-48h], 0
    jmp qword ptr [ASM_cube__Creature__OnPlayerDrownDeath_JMPBACK]
ASM_cube__Creature__OnPlayerDrownDeath ENDP

ASM_cube__Creature__OnPlayerFallDeath PROC
    mov rcx, r13
    call cube__Creature__OnPlayerFallDeath
    jmp qword ptr [ASM_cube__Creature__OnPlayerFallDeath_JMPBACK]
ASM_cube__Creature__OnPlayerFallDeath ENDP

ASM_cube__Game__MouseUp PROC
    PUSH_ALL
    PREPARE_STACK
    call cube__Game__MouseUp
    RESTORE_STACK
    POP_ALL
    xor r12d, r12d
    jmp qword ptr [ASM_cube__Game__MouseUp_jmpback]
ASM_cube__Game__MouseUp ENDP

ASM_cube__Game__Update PROC
    PUSH_ALL
    mov rcx, r13
    PREPARE_STACK
    call cube__Game__Update
    RESTORE_STACK
    POP_ALL
    mov rdx, [r13+8h]
    mov eax, [rdx+220h]
    cmp [r13+206Ch], eax
    jnz update_bail
    jmp qword ptr [ASM_cube__Game__Update_jmpback]
update_bail:
    jmp qword ptr [ASM_cube__Game__Update_bail]
ASM_cube__Game__Update ENDP

ASM_cube__Item__GetBuyingPrice PROC
    movaps xmm0, xmm6
    movaps xmm6, xmmword ptr [rsp+20h]
    push rax
    mov rdx, r8
    lea r8, [rsp]
    PREPARE_STACK
    call cube__Item__GetBuyingPrice
    RESTORE_STACK
    pop rax
    add rsp, 38h
    ret
ASM_cube__Item__GetBuyingPrice ENDP

END
