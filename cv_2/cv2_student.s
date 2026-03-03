.intel_syntax noprefix

.text

# ==============================================================================
# FUNKCE: brightness_mmx
# POPIS:  Zvýší jas obrázku pomocí instrukcí MMX.
# PLATFORMA: Windows x64 (Microsoft ABI)
#
# REGISTRY (VSTUP):
#   RCX = uint8_t* data
#   RDX = size_t count
#   R8  = int brightness
# ==============================================================================
.global brightness_mmx
brightness_mmx:
    push rbp
    mov rbp, rsp

    xor rax, rax
    movzx r8d, r8b
    movd  mm1, r8d
    punpcklbw mm1, mm1
    punpcklwd mm1, mm1
    pshufw    mm1, mm1, 0

.L_vector_loop:
    movq mm0, [rcx + rax]

    paddusb mm0, mm1

    movq  [rcx + rax], mm0


    add rax, 8

    cmp rax, rdx           
    jge .L_vector_end

    jmp .L_vector_loop

.L_vector_end:
    emms
    mov rsp, rbp
    pop rbp
    ret


# ==============================================================================
# FUNKCE: brightness_scalar_asm
# POPIS:  Referenční implementace pro Windows x64.
# ==============================================================================
.global brightness_scalar_asm
brightness_scalar_asm:
    push rbp
    mov rbp, rsp
    
    xor rax, rax

.L_scalar_loop:
    cmp rax, rdx            # Porovnání s count (RDX)
    jge .L_scalar_end

    movzx r9d, byte ptr [rcx + rax] # Načtení z RCX
    add r9d, r8d                    # Přičtení jasu (R8)
    
    # Saturace
    cmp r9d, 255
    jle .L_scalar_store
    mov r9d, 255

.L_scalar_store:
    mov [rcx + rax], r9b
    inc rax
    jmp .L_scalar_loop

.L_scalar_end:
    mov rsp, rbp
    pop rbp
    ret
