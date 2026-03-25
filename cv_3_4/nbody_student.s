.intel_syntax noprefix

.data
    # Definice vlastních konstant v paměti
    # (například masky pro vmaskmovps, nebo vektory jedniček/nul)
    # 
    # Příklad:
    # .balign 32
    # my_mask: .long 0, 0, 0, 0, 0, 0, 0, 0
    SPHERS_COUNT:   .int 512
    FOUR:           .int 4
    FIVE:           .int 5
    NUL:           .int 0


    .equ OFF_X, 0
    .equ OFF_Y, 2048
    .equ OFF_Z, 4096
    .equ OFF_R, 6144
    .equ OFF_M, 8192
    .equ OFF_vX, 10240
    .equ OFF_vY, 12288
    .equ OFF_vZ, 14336
    .equ OFF_color, 16384

    .balign 16
    two: .float 2.0, 2.0, 2.0, 2.0


.text
.global nbody_simd

# C++ signatura: 
# extern "C" void nbody_simd(t_sphere_array * _spheres, int count, float dt);

nbody_simd:
    # =========================================================================
    # WINDOWS x64 CALLING CONVENTION (DŮLEŽITÉ!)
    # =========================================================================
    # Parametry jsou předávány následovně:
    # 1. parametr (_spheres pointer) -> registr RCX
    # 2. parametr (count int)        -> registr RDX
    # 3. parametr (dt float)         -> registr XMM2 (Pozor! Ne XMM0, ale XMM2!)
    # =========================================================================

    # Prologue (pokud by studenti potřebovali ukládat callee-saved registry)
    # push rbp
    # mov rbp, rsp

    # -------------------------------------------------------------------------
    #  DOPLNIT KÓD PRO SIMD VÝPOČET (N-BODY PROBLEM)
    # -------------------------------------------------------------------------
    
    sub rsp, 168

    movaps [rsp + 0],   xmm6
    movaps [rsp + 16],  xmm7
    movaps [rsp + 32],  xmm8
    movaps [rsp + 48],  xmm9
    movaps [rsp + 64],  xmm10
    movaps [rsp + 80],  xmm11
    movaps [rsp + 96],  xmm12
    movaps [rsp + 112], xmm13
    movaps [rsp + 128], xmm14
    movaps [rsp + 144], xmm15


    xor r8d, r8d

  #  mov r10d, edx
  #  sub r10d, 3

    # r8d -> i
    # r9d -> j

loop_i:

    cmp r8d, edx
    jge done

    movss xmm0, [RCX + r8*4 + OFF_X]            
    movss xmm1, [RCX + r8*4 + OFF_Y]
    movss xmm3, [RCX + r8*4 + OFF_Z]
    movss xmm7, [RCX + r8*4 + OFF_R]

    shufps xmm0, xmm0, 0
    shufps xmm1, xmm1, 0
    shufps xmm3, xmm3, 0
    shufps xmm7, xmm7, 0
    # xi,xi,xi,xi
    # všude je 1 prvek

    lea r9d, [r8d + 1]
    # j = i + 1

loop_j:

    cmp r9d, edx
    jge done_j


    movups xmm4, [RCX + r9*4 + OFF_X]            
    movups xmm5, [RCX + r9*4 + OFF_Y]
    movups xmm6, [RCX + r9*4 + OFF_Z]   
    movups xmm8, [RCX + r9*4 + OFF_R]

    movaps xmm10, xmm0
    movaps xmm11, xmm1
    movaps xmm12, xmm3

    subps xmm0, xmm4  # xmm4 = dx
    subps xmm1, xmm5  # xmm5 = dy  vše je tady po 4 po sobě jdoucích 
    subps xmm3, xmm6  # xmm6 = dz
    # na 1 "i" beru 4 "j"

    movaps xmm4, xmm0
    movaps xmm5, xmm1
    movaps xmm6, xmm3

    movaps xmm0, xmm10
    movaps xmm1, xmm11
    movaps xmm3, xmm12

    movaps xmm10, xmm4
    movaps xmm11, xmm5
    movaps xmm12, xmm6

    # vector_length = sqrt(dx*dx + dy*dy + dz*dz) pro 4 lane v xmm4

    mulps xmm4, xmm4
    mulps xmm5, xmm5
    mulps xmm6, xmm6

    addps xmm4, xmm5
    addps xmm4, xmm6

    sqrtps xmm4, xmm4                  # xmm4 = vector_length

    movaps xmm14, xmm4                 # kopie délky
    xorps xmm15, xmm15                 # 0.0
    cmpps xmm15, xmm14, 1              # xmm15 = (0.0 < vector_length)

    addps xmm8, xmm7                   # xmm8 = r_i + r_j
    cmpps xmm14, xmm8, 1               # xmm14 = (vector_length < r_i + r_j)

    andps xmm14, xmm15                 # jen kolize a zároveň délka > 0
    movaps xmm8, xmm14                 # finální maska do xmm8

    movmskps eax, xmm8
    test eax, eax
    jz skip_crash


    divps xmm10, xmm4
    divps xmm11, xmm4
    divps xmm12, xmm4

 #               float a1_dot = spheres.vectorX[i] * n_x_norm + spheres.vectorY[i] * n_y_norm + spheres.vectorZ[i] * n_z_norm;
 #               float a2_dot = spheres.vectorX[j] * n_x_norm + spheres.vectorY[j] * n_y_norm + spheres.vectorZ[j] * n_z_norm;

    movups xmm5, [RCX + r9*4 + OFF_vY]
    movups xmm6, [RCX + r9*4 + OFF_vZ]
    movups xmm9, [RCX + r9*4 + OFF_vX]

    mulps xmm5, xmm11
    mulps xmm6, xmm12
    mulps xmm9, xmm10

    addps xmm9, xmm5
    addps xmm9, xmm6

    movss xmm5, [RCX + r8*4 + OFF_vY]
    shufps xmm5, xmm5, 0
    movss xmm6, [RCX + r8*4 + OFF_vZ]
    shufps xmm6, xmm6, 0

    mulps xmm5, xmm11
    mulps xmm6, xmm12
    addps xmm5, xmm6

    movss xmm6, [RCX + r8*4 + OFF_vX]
    shufps xmm6, xmm6, 0
    mulps xmm6, xmm10

    addps xmm5, xmm6

# float P = (2.0f * (a1_dot - a2_dot)) / (spheres.m[i] + spheres.m[j]);

    subps xmm5, xmm9                  # xmm5 = a1_dot - a2_dot

    movups xmm6, [RCX + r9*4 + OFF_M] # xmm6 = m[j..j+3], necháš si ho i pro další výpočty

    movss xmm13, [RCX + r8*4 + OFF_M] # xmm13 = m[i]
    shufps xmm13, xmm13, 0            # xmm13 = [m[i], m[i], m[i], m[i]]

    movaps xmm9, xmm6                 # xmm9 = m[j]
    addps xmm9, xmm13                 # xmm9 = m[i] + m[j]

    movaps xmm14, XMMWORD PTR [rip + two]
    mulps xmm5, xmm14                 # xmm5 = 2.0f * (a1_dot - a2_dot)

    divps xmm5, xmm9                  # xmm5 = P

# spheres.vectorX[i] -= sum_over_lanes( P * m[j] * n_x_norm )

    movaps xmm15, xmm5
    mulps xmm15, xmm6
    mulps xmm15, xmm10
    andps xmm15, xmm8

    # horizontální součet 4 lane do xmm15[0]
    movaps xmm14, xmm15
    movhlps xmm14, xmm15
    addps xmm15, xmm14

    movaps xmm14, xmm15
    shufps xmm14, xmm14, 1
    addss xmm15, xmm14

    movss xmm14, [RCX + r8*4 + OFF_vX]
    subss xmm14, xmm15
    movss [RCX + r8*4 + OFF_vX], xmm14

# spheres.vectorY[i] -= sum_over_lanes( P * m[j] * n_y_norm )

    movaps xmm15, xmm5
    mulps xmm15, xmm6
    mulps xmm15, xmm11
    andps xmm15, xmm8

    # horizontální součet 4 lane do xmm15[0]
    movaps xmm14, xmm15
    movhlps xmm14, xmm15
    addps xmm15, xmm14

    movaps xmm14, xmm15
    shufps xmm14, xmm14, 1
    addss xmm15, xmm14

    movss xmm14, [RCX + r8*4 + OFF_vY]
    subss xmm14, xmm15
    movss [RCX + r8*4 + OFF_vY], xmm14

# spheres.vectorZ[i] -= sum_over_lanes( P * m[j] * n_z_norm )

    movaps xmm15, xmm5
    mulps xmm15, xmm6
    mulps xmm15, xmm12
    andps xmm15, xmm8

    # horizontální součet 4 lane do xmm15[0]
    movaps xmm14, xmm15
    movhlps xmm14, xmm15
    addps xmm15, xmm14

    movaps xmm14, xmm15
    shufps xmm14, xmm14, 1
    addss xmm15, xmm14

    movss xmm14, [RCX + r8*4 + OFF_vZ]
    subss xmm14, xmm15
    movss [RCX + r8*4 + OFF_vZ], xmm14



#   spheres.vectorX[j] += P * spheres.m[i] * n_x_norm;

    movups xmm14, [RCX + r9*4 + OFF_vX]
    movaps xmm9, xmm14
    movaps xmm15, xmm5

    mulps xmm15, xmm13
    mulps xmm15, xmm10

    addps xmm14, xmm15  ## NOVÝ VEKTOR

    # maskování
    movaps xmm4, xmm8 # kopie masky
    andps xmm14, xmm4 # maskování nových
    andnps xmm4, xmm9 # maskování starých
    orps xmm14, xmm4 # spojení starého a nového


    movups [RCX + r9*4 + OFF_vX], xmm14

# spheres.vectorY[j] += P * spheres.m[i] * n_y_norm;

    movups xmm14, [RCX + r9*4 + OFF_vY]
    movaps xmm9, xmm14
    movaps xmm15, xmm5

    mulps xmm15, xmm13
    mulps xmm15, xmm11

    addps xmm14, xmm15  ## NOVÝ VEKTOR

    # maskování
    movaps xmm4, xmm8 # kopie masky
    andps xmm14, xmm4 # maskování nových
    andnps xmm4, xmm9 # maskování starých
    orps xmm14, xmm4 # spojení starého a nového


    movups [RCX + r9*4 + OFF_vY], xmm14

#   spheres.vectorZ[j] += P * spheres.m[i] * n_z_norm;

    movups xmm14, [RCX + r9*4 + OFF_vZ]
    movaps xmm9, xmm14
    movaps xmm15, xmm5

    mulps xmm15, xmm13
    mulps xmm15, xmm12

    addps xmm14, xmm15  ## NOVÝ VEKTOR

    # maskování
    movaps xmm4, xmm8 # kopie masky
    andps xmm14, xmm4 # maskování nových
    andnps xmm4, xmm9 # maskování starých
    orps xmm14, xmm4 # spojení starého a nového


    movups [RCX + r9*4 + OFF_vZ], xmm14


    # spheres.color[i] = green;

    mov dword ptr [RCX + r8*4 + OFF_color], 2

skip_crash:

    add r9d, 4
    jmp loop_j

done_j:

    


# nová poloha

    movss xmm14, [RCX + r8*4 + OFF_vX]
    mulss xmm14, xmm2
    addss xmm0, xmm14
    movss [RCX + r8*4 + OFF_X], xmm0

    movss xmm14, [RCX + r8*4 + OFF_vY]
    mulss xmm14, xmm2
    addss xmm1, xmm14
    movss [RCX + r8*4 + OFF_Y], xmm1

    movss xmm14, [RCX + r8*4 + OFF_vZ]
    mulss xmm14, xmm2
    addss xmm3, xmm14
    movss [RCX + r8*4 + OFF_Z], xmm3





##### tady asi nic

    inc r8d
    jmp loop_i



done:
    
    # -------------------------------------------------------------------------

    # Úklid stavu AVX registrů (dobrý zvyk při použití YMM registrů)
    vzeroupper

    # Epilogue
    # pop rbp

    # Návrat z funkce (Na x64 Windows se parametry ze zásobníku neuklízí přes ret X)

    movaps xmm6,  [rsp + 0]
    movaps xmm7,  [rsp + 16]
    movaps xmm8,  [rsp + 32]
    movaps xmm9,  [rsp + 48]
    movaps xmm10, [rsp + 64]
    movaps xmm11, [rsp + 80]
    movaps xmm12, [rsp + 96]
    movaps xmm13, [rsp + 112]
    movaps xmm14, [rsp + 128]
    movaps xmm15, [rsp + 144]

    add rsp, 168

    ret