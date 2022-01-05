import Lean

open Lean
open Lean.Meta
def tst (declName : Name) : MetaM Unit := do
  IO.println (← getEqnsFor? declName)

mutual
def g (i j : Nat) : Nat :=
  if i < 5 then 0 else
  match j with
  | Nat.zero => 1
  | Nat.succ j => h i j
def h (i j : Nat) : Nat :=
  match j with
  | 0 => g i 0
  | Nat.succ j => g i j
end
termination_by
 invImage
    (fun
      | Sum.inl ⟨_, n⟩ => (n, 0)
      | Sum.inr ⟨_, n⟩ => (n, 1))
    (Prod.lex sizeOfWFRel sizeOfWFRel)
decreasing_by
  simp [invImage, InvImage, Prod.lex, sizeOfWFRel, measure, Nat.lt_wfRel]
  first
  | apply Prod.Lex.left
    apply Nat.lt_succ_self
  | apply Prod.Lex.right
    decide

#eval tst ``g
#check g.eq_1
#check g.eq_2
#check g.eq_3
#eval tst ``h
#check h.eq_1
#check h.eq_2
