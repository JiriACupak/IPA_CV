.intel_syntax noprefix

.data
    # --- Konstanty (32-bit float) ---
    gravity:    .float 0.3
    thrust:     .float 0.8
    groundY:    .float -0.9
    maxSafeVel: .float -0.3     # Limit pro bezpecne pristani
    fuelCons:   .float 20.0
    zero:       .float 0.0

    # --- Offsety ve strukture LanderState ---
    .equ OFF_POSY, 0
    .equ OFF_VELY, 4
    .equ OFF_FUEL, 8
    .equ OFF_THRUST, 12
    .equ OFF_STATUS, 16

.text
.global updateLander

# void updateLander(LanderState* state, float dt)
# RCX  = pointer na strukturu 'state'
# XMM1 = dt (Windows x64 volaci konvence predava float v XMM1)

updateLander:
    push rbp
    mov rbp, rsp
    
    # -------------------------------------------------------------
    # 0. PRIPRAVA DT
    # x87 FPU neumi cist XMM registry. Musime DT presunout do pameti.
    # -------------------------------------------------------------
    sub rsp, 8
    movss dword ptr [rsp], xmm1   # Uloz DT na stack CPU (lokalni promenna)

    # -------------------------------------------------------------
    # 1. GRAVITACE: velY = velY - (gravity * dt)
    # -------------------------------------------------------------
    fld dword ptr [rcx + OFF_VELY]  # ST0 = velY
    fld dword ptr [rip + gravity]   # ST0 = 0.3, ST1 = velY
    fmul dword ptr [rsp]            # ST0 = 0.3 * dt
    
    # Odcitani: ST1 (velY) - ST0 (grav*dt) -> vysledek do ST1, pop ST0
    fsubp st(1), st(0)              # ST0 = nova rychlost
    
    # Ulozeni a vycisteni stacku
    fstp dword ptr [rcx + OFF_VELY] # Stack je nyni prazdny

    # -------------------------------------------------------------
    # 2. MOTOR A PALIVO
    # -------------------------------------------------------------
    cmp dword ptr [rcx + OFF_THRUST], 1
    jne skip_thrust

    # Kontrola paliva: if (fuel > 0)
    fld dword ptr [rcx + OFF_FUEL]  # ST0 = fuel
    fldz                            # ST0 = 0.0, ST1 = fuel
    
    # Porovnani: ST1 (fuel) vs ST0 (0.0). 
    # fcomip porovna a POPne ST0 (nulu).
    fcomip st(0), st(1)             # Porovnava 0.0 vs fuel? Ne, ST0 vs ST1.
    # POZOR: fcomip porovnava ST0 s operandem.
    # ST0 (0.0) - ST1 (Fuel).
    # Pokud Fuel=10: 0 - 10 = zaporne (CF=1). -> JB (Jump Below)
    # Pokud Fuel=-5: 0 - (-5) = kladne (CF=0). -> JAE
    
    # Ale musime jeste vycistit ST1 (fuel), ktery tam zustal po fcomip (ten popnul jen ST0)
    fstp st(0)                      # Vycisti Fuel ze stacku. Stack prazdny.

    # Nyni testujeme flagy z fcomip:
    jnc skip_thrust                 # Pokud neni Carry (CF=0), znamena to 0 >= Fuel. Konec.
                                    # Pokud Carry (CF=1), znamena to 0 < Fuel. Letime.

    # --- Aplikace tahu: velY += thrust * dt ---
    fld dword ptr [rcx + OFF_VELY]
    fld dword ptr [rip + thrust]
    fmul dword ptr [rsp]            # thrust * dt
    faddp st(1), st(0)              # velY + narust
    fstp dword ptr [rcx + OFF_VELY]

    # --- Spotreba paliva: fuel -= fuelCons * dt ---
    fld dword ptr [rcx + OFF_FUEL]
    fld dword ptr [rip + fuelCons]
    fmul dword ptr [rsp]
    fsubp st(1), st(0)              # fuel - spotreba
    fstp dword ptr [rcx + OFF_FUEL]

skip_thrust:

    # -------------------------------------------------------------
    # 3. POZICE: posY = posY + velY * dt
    # -------------------------------------------------------------
    # Doplnit

    fld dword ptr [rcx + OFF_POSY]
    fld dword ptr [rcx + OFF_VELY]
    fmul dword ptr [rsp]            # thrust * dt
    faddp st(1), st(0)              # velY + narust
    fstp dword ptr [rcx + OFF_POSY]

    # -------------------------------------------------------------
    # 4. KOLIZE SE ZEMI
    # -------------------------------------------------------------
    
    # Doplnit

    fld dword ptr [rcx + OFF_POSY]
    fld dword ptr [rip + groundY]
    fcomip st(0), st(1)  
    fstp st(0)     
    jae dotyk
    jmp end_func

dotyk:
    fld dword ptr [rcx + OFF_VELY]
    fld dword ptr [rip + maxSafeVel]
    fcomip st(0), st(1)  
    fstp st(0)     
    jae crash


    # --- USPECH ---
    mov dword ptr [rcx + OFF_STATUS], 1
    jmp clean_stack_and_ret

crash:
    mov dword ptr [rcx + OFF_STATUS], 2

clean_stack_and_ret:
end_func:
    # Uklid lokalnich promennych (dt)
    add rsp, 8
    pop rbp
    ret