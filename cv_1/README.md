# Laboratorní cvičení: ASM Lunar Lander (x87 FPU)

**Téma:** Koprocesor x87, Floating Point, Volací konvence x64, DLL

---

## 1. Cíle cvičení
1.  Zopakovat si práci s **zásobníkem koprocesoru x87** (FPU), seznámit se **DLL**.
2.  Naučit se kombinovat **C++ a Asembler** pomocí DLL.
3.  Zvládnout **volací konvenci Windows x64/Linux ABI** pro desetinná čísla.
4.  Implementovat jednoduchou fyziku pohybu a detekci kolizí.

## 2. Teoretický úvod

### 2.1 Koprocesor x87 (FPU)
Pro výpočty s desetinnými čísly (`float`, `double`) budeme používat starší, ale pro výuku skvělou architekturu **x87**. Ta nepoužívá klasické registry, ale **zásobník** osmi registrů `ST(0)` až `ST(7)`.
*   **`fld` (Load):** Vloží hodnotu na vrchol zásobníku (`push`).
*   **`fstp` (Store & Pop):** Uloží vrchol zásobníku do paměti a odstraní ho (`pop`).
*   **`fadd`, `fsub`, `fmul`:** Aritmetické operace (často ve formátu `faddp st(1), st(0)`).

### 2.2 Struktura dat (Memory Layout)
V C++ i ASM sdílíme následující strukturu. V Asembleru neexistují názvy položek, musíte používat **offsety** (vzdálenost v bajtech od začátku struktury).

| C++ Typ | Název | Offset (ASM) | Popis |
| :--- | :--- | :--- | :--- |
| `float` | `posY` | **0** | Výška (0.0 = střed, -1.0 = zem) |
| `float` | `velY` | **4** | Rychlost (kladná nahoru, záporná dolů) |
| `float` | `fuel` | **8** | Zbývající palivo |
| `int` | `isThrusting` | **12** | 1 = motor zapnut, 0 = vypnut |
| `int` | `status` | **16** | 0 = letí, 1 = přistál, 2 = havárie |

### 2.3 Volací konvence (Důležité!)
Na 64bitových Windows se první `float` parametr funkce předává v registru **`XMM1`**.
**Problém:** Koprocesor x87 neumí číst přímo z `XMM` registrů.
**Řešení:** Hodnotu musíte nejprve uložit na zásobník CPU (do paměti RAM) a teprve poté ji načíst do FPU.

```asm
sub rsp, 8             ; Udělej místo na stacku
movss [rsp], xmm1      ; Přesuň XMM1 do paměti
fld dword ptr [rsp]    ; Načti z paměti do FPU
```

---

## 3. Zadání úlohy

Vaším úkolem je implementovat funkci `updateLander` v souboru `cv1_student.s`. Funkce je volána každých 16 ms a stará se o fyziku modulu.

### Krok 1: Gravitace
Modul je neustále přitahován k zemi.
*   Rovnice: $velY = velY - (gravity \cdot dt)$
*   *Nápověda:* Načtěte `velY`, načtěte `gravity`, načtěte `dt`, vynásobte a odečtěte. Nezapomeňte výsledek uložit zpět (`fstp`).

### Krok 2: Motor a Palivo
Pokud hráč drží mezerník (`isThrusting == 1`) a má palivo (`fuel > 0`):
1.  Zvyšte rychlost: $velY = velY + (thrust \cdot dt)$
2.  Snižte palivo: $fuel = fuel - (fuelCons \cdot dt)$
3.  *Pozor:* Porovnávání floatů (`fcomip`) vyžaduje opatrnost a následný úklid zásobníku.

### Krok 3: Pohyb
Aktualizujte pozici podle rychlosti.
*   Rovnice: $posY = posY + (velY \cdot dt)$

### Krok 4: Kolize a detekce přistání
Zkontrolujte, zda modul nenarazil do země (`posY <= -0.9`).
Pokud ano:
1.  Zastavte modul (nastavte `posY = -0.9`).
2.  Zkontrolujte rychlost dopadu.
    *   Pokud je rychlost "měkká" (větší než `-0.3`), nastavte `status = 1` (Výhra).
    *   Jinak nastavte `status = 2` (Havárie).

---

## 4. Tahák instrukcí (Cheat Sheet)

| Instrukce | Operandy | Popis |
| :--- | :--- | :--- |
| `fld [mem]` | `float` | Načte číslo z paměti na ST(0). |
| `fstp [mem]` | `float` | Uloží ST(0) do paměti a **vyhodí** ho ze zásobníku. |
| `faddp st(1), st(0)` | - | Sečte ST(1) + ST(0), výsledek v ST(1), POP ST(0). |
| `fsubp st(1), st(0)` | - | Odečte ST(1) - ST(0), výsledek v ST(1), POP ST(0). |
| `fmul [mem]` | `float` | Vynásobí ST(0) * hodnota v paměti. |
| `fcomip st(0), st(1)` | - | Porovná ST(0) vs ST(1), nastaví EFLAGS, POP ST(0). |
| `ja / jb` | label | Skok "Above" (větší) / "Below" (menší) - po `fcomip`. |
