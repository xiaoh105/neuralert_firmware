; ModuleID = 'notify_user_led.bc'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:32:32-Fi8-i64:64-v128:64:128-a:0:32-n32-S64"
target triple = "thumbv7em-unknown-none-eabihf"

%struct._userData = type { i32, i32, i32, i64, i64, i32, i32, i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i64, i64, i16, [128 x i8], [7 x i8], i32, i32, i32, i32, i32, [4 x i32], i32, i32, i32, [3 x i32], i16, i8, i32, i32, [973 x i32], i16, i16, i32, i32, i32, i32 }
%struct.ledColorStruct = type { i8, i8, i16, i8, i8, i16, i16, i16, i8, i16 }

@processLists = hidden unnamed_addr global i32 0, align 4
@pUserData = hidden global %struct._userData* null, align 4
@ledColor = dso_local local_unnamed_addr global %struct.ledColorStruct zeroinitializer, align 2

; Function Attrs: noinline nounwind optsize
define hidden fastcc void @notify_user_LED() unnamed_addr #0 {
  %1 = load i32, i32* @processLists, align 4, !tbaa !8
  %2 = icmp eq i32 %1, 0
  br i1 %2, label %8, label %3

3:                                                ; preds = %0
  %4 = and i32 %1, 256
  %5 = icmp eq i32 %4, 0
  br i1 %5, label %7, label %6

6:                                                ; preds = %3
  tail call void @setLEDState(i8 noundef zeroext 6, i8 noundef zeroext 3, i16 noundef zeroext 200, i8 noundef zeroext 0, i8 noundef zeroext 0, i16 noundef zeroext 0, i16 noundef zeroext 300) #2
  br label %9

7:                                                ; preds = %3
  tail call void @setLEDState(i8 noundef zeroext 0, i8 noundef zeroext 0, i16 noundef zeroext 0, i8 noundef zeroext 0, i8 noundef zeroext 0, i16 noundef zeroext 0, i16 noundef zeroext 200) #2
  br label %9

8:                                                ; preds = %0
  tail call void @setLEDState(i8 noundef zeroext 0, i8 noundef zeroext 0, i16 noundef zeroext 0, i8 noundef zeroext 0, i8 noundef zeroext 0, i16 noundef zeroext 0, i16 noundef zeroext 200) #2
  br label %9

9:                                                ; preds = %6, %7, %8
  ret void
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind optsize willreturn writeonly
define dso_local void @setLEDState(i8 noundef zeroext %0, i8 noundef zeroext %1, i16 noundef zeroext %2, i8 noundef zeroext %3, i8 noundef zeroext %4, i16 noundef zeroext %5, i16 noundef zeroext %6) local_unnamed_addr #1 {
  store i8 %0, i8* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 0), align 2, !tbaa !12
  store i8 %1, i8* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 1), align 1, !tbaa !15
  store i16 %2, i16* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 2), align 2, !tbaa !16
  store i16 %2, i16* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 6), align 2, !tbaa !17
  store i8 %3, i8* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 3), align 2, !tbaa !18
  store i8 %4, i8* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 4), align 1, !tbaa !19
  store i16 %5, i16* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 5), align 2, !tbaa !20
  store i16 %5, i16* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 7), align 2, !tbaa !21
  store i8 0, i8* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 8), align 2, !tbaa !22
  store i16 %6, i16* getelementptr inbounds (%struct.ledColorStruct, %struct.ledColorStruct* @ledColor, i32 0, i32 9), align 2, !tbaa !23
  ret void
}

; attributes #0 = { noinline nounwind optsize "frame-pointer"="all" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cortex-m4" "target-features"="+armv7e-m,+dsp,+fp16,+hwdiv,+strict-align,+thumb-mode,+vfp2sp,+vfp3d16sp,+vfp4d16sp,-aes,-bf16,-cdecp0,-cdecp1,-cdecp2,-cdecp3,-cdecp4,-cdecp5,-cdecp6,-cdecp7,-crc,-crypto,-d32,-dotprod,-fp-armv8,-fp-armv8d16,-fp-armv8d16sp,-fp-armv8sp,-fp16fml,-fp64,-fullfp16,-hwdiv-arm,-i8mm,-lob,-mve,-mve.fp,-neon,-pacbti,-ras,-sb,-sha2,-vfp2,-vfp3,-vfp3d16,-vfp3sp,-vfp4,-vfp4d16,-vfp4sp" }
; attributes #1 = { mustprogress nofree noinline norecurse nosync nounwind optsize willreturn writeonly "frame-pointer"="all" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cortex-m4" "target-features"="+armv7e-m,+dsp,+fp16,+hwdiv,+strict-align,+thumb-mode,+vfp2sp,+vfp3d16sp,+vfp4d16sp,-aes,-bf16,-cdecp0,-cdecp1,-cdecp2,-cdecp3,-cdecp4,-cdecp5,-cdecp6,-cdecp7,-crc,-crypto,-d32,-dotprod,-fp-armv8,-fp-armv8d16,-fp-armv8d16sp,-fp-armv8sp,-fp16fml,-fp64,-fullfp16,-hwdiv-arm,-i8mm,-lob,-mve,-mve.fp,-neon,-pacbti,-ras,-sb,-sha2,-vfp2,-vfp3,-vfp3d16,-vfp3sp,-vfp4,-vfp4d16,-vfp4sp" }
; attributes #2 = { nobuiltin nounwind optsize "no-builtins" }

!llvm.ident = !{!0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0, !0}
!llvm.module.flags = !{!1, !2, !3, !4, !5, !6, !7}

!0 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 1, !"min_enum_size", i32 4}
!3 = !{i32 1, !"branch-target-enforcement", i32 0}
!4 = !{i32 1, !"sign-return-address", i32 0}
!5 = !{i32 1, !"sign-return-address-all", i32 0}
!6 = !{i32 1, !"sign-return-address-with-bkey", i32 0}
!7 = !{i32 7, !"frame-pointer", i32 2}
!8 = !{!9, !9, i64 0}
!9 = !{!"int", !10, i64 0}
!10 = !{!"omnipotent char", !11, i64 0}
!11 = !{!"Simple C/C++ TBAA"}
!12 = !{!13, !10, i64 0}
!13 = !{!"", !10, i64 0, !10, i64 1, !14, i64 2, !10, i64 4, !10, i64 5, !14, i64 6, !14, i64 8, !14, i64 10, !10, i64 12, !14, i64 14}
!14 = !{!"short", !10, i64 0}
!15 = !{!13, !10, i64 1}
!16 = !{!13, !14, i64 2}
!17 = !{!13, !14, i64 8}
!18 = !{!13, !10, i64 4}
!19 = !{!13, !10, i64 5}
!20 = !{!13, !14, i64 6}
!21 = !{!13, !14, i64 10}
!22 = !{!13, !10, i64 12}
!23 = !{!13, !14, i64 14}