; ModuleID = 'assign.c'
; source_filename = "assign.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; main
define dso_local i32 @main() #0 {
    ; int a[10];
    ; %1 = common dso_local [10 x i32] zeroinitializer, align 4
    ; why align 16?
    %1 = alloca [10 x i32], align 16
    ; https://stackoverflow.com/questions/9413376/what-is-wrong-with-this-llvm-ir-code
    ; ; a[0] = 10;
    ; store i32 10, i32* getelementptr inbounds ([10 x i32], [10 x i32]* %1, i64 0, i64 0), align 16
    ; ; a[1] = a[0] * 2;
    ; %2 = load i32, i32* getelementptr inbounds ([10 x i32], [10 x i32]* %1, i64 0, i64 0), align 16
    ; %3 = mul i32 %2, 2
    ; store i32 %3, i32* getelementptr inbounds ([10 x i32], [10 x i32]* %1, i64 0, i64 1), align 16
    ; ; return a[1];
    ; %4 = load i32, i32* getelementptr inbounds ([10 x i32], [10 x i32]* %1, i64 0, i64 1), align 16
    ; ret i32 %4

    ; a[0] = 10;
    %2 = getelementptr inbounds [10 x i32], [10 x i32]* %1, i64 0, i64 0
    store i32 10, i32* %2
    ; a[1] = a[0] * 2;
    ; getelementptr again better
    %3 = getelementptr inbounds [10 x i32], [10 x i32]* %1, i64 0, i64 0
    %4 = load i32, i32* %3
    %5 = mul i32 %4, 2
    %6 = getelementptr inbounds [10 x i32], [10 x i32]* %1, i64 0, i64 1
    store i32 %5, i32* %6
    ; return a[1];
    %7 = getelementptr inbounds [10 x i32], [10 x i32]* %1, i64 0, i64 1
    %8 = load i32, i32* %7
    ret i32 %8
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}