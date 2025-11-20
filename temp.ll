; ModuleID = 'main_module'
source_filename = "main_module"

%Food = type { i32, i32 }
%SegT = type { i32, i32, i32, i32, %Food }
%Node = type { [201 x i32], i32, i32, [11 x i32], i32, i32 }
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare void @exit(i32)
declare void @llvm.memset.p0.i64(i8*, i8, i64, i1)
declare void @llvm.memcpy.p0.p0.i64(i8*, i8*, i64, i1)

@.str.int = constant [3 x i8] c"%d\00"
@.str.int_newline = constant [4 x i8] c"%d\0A\00"
@.str.int_scanf = constant [3 x i8] c"%d\00"

@MAXN = constant i32 200
@LOG_MAXN = constant i32 10
@MAXSEG = constant i32 2000

define void @Food_better(%Food* %sret_ptr, %Food* %self, %Food* %other) {
bb.entry:
  %stack.0 = alloca %Food
  %stack.1 = alloca %Food
  store %Food %self, %Food* %stack.0
  %0 = bitcast %Food* %stack.1 to i8*
  %1 = bitcast %Food* %other to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %0, i8* %1, i64 8, i1 false)
  %2 = getelementptr inbounds %Food, %Food* %stack.0, i32 0, i32 0
  %3 = load i32, i32* %2
  %4 = getelementptr inbounds %Food, %Food* %stack.1, i32 0, i32 0
  %5 = load i32, i32* %4
  %6 = icmp eq i32 %3, %5
  br i1 %6, label %if.then.0, label %if.else.0
if.then.0:
  %7 = getelementptr inbounds %Food, %Food* %stack.0, i32 0, i32 1
  %8 = load i32, i32* %7
  %9 = getelementptr inbounds %Food, %Food* %stack.1, i32 0, i32 1
  %10 = load i32, i32* %9
  %11 = icmp slt i32 %8, %10
  br i1 %11, label %if.then.1, label %if.else.1
if.then.1:
  br label %if.end.1
if.else.1:
  br label %if.end.1
if.end.1:
  %12 = phi %Food* [%stack.0, %if.then.1], [%stack.1, %if.else.1]
  br label %if.end.0
if.else.0:
  %13 = getelementptr inbounds %Food, %Food* %stack.0, i32 0, i32 0
  %14 = load i32, i32* %13
  %15 = getelementptr inbounds %Food, %Food* %stack.1, i32 0, i32 0
  %16 = load i32, i32* %15
  %17 = icmp sgt i32 %14, %16
  br i1 %17, label %if.then.2, label %if.else.2
if.then.2:
  br label %if.end.2
if.else.2:
  br label %if.end.2
if.end.2:
  %18 = phi %Food* [%stack.0, %if.then.2], [%stack.1, %if.else.2]
  br label %if.end.0
if.end.0:
  %19 = phi %Food* [%12, %if.end.1], [%18, %if.end.2]
  ret void
}


define i32 @new_segt([2000 x %SegT]* noalias %seg_pool, i32* noalias %seg_cnt, i32 %l, i32 %r, %Food* %val) {
bb.entry:
  %stack.0 = alloca i32
  %stack.1 = alloca i32
  %stack.2 = alloca %Food
  %stack.3 = alloca %SegT
  store i32 %l, i32* %stack.0
  store i32 %r, i32* %stack.1
  %0 = bitcast %Food* %stack.2 to i8*
  %1 = bitcast %Food* %val to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %0, i8* %1, i64 8, i1 false)
  %2 = load i32, i32* %seg_cnt
  %3 = add i32 %2, 1
  store i32 %3, i32* %seg_cnt
  %4 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 0
  %5 = load i32, i32* %stack.0
  store i32 %5, i32* %4
  %6 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 1
  %7 = load i32, i32* %stack.1
  store i32 %7, i32* %6
  %8 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 2
  store i32 0, i32* %8
  %9 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 3
  store i32 0, i32* %9
  %10 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 4
  %11 = load %Food, %Food* %stack.2
  store %Food %11, %Food* %10
  %12 = load i32, i32* %seg_cnt
  %13 = zext i32 %12 to i64
  %14 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %13
  %15 = bitcast %SegT* %14 to i8*
  %16 = bitcast %SegT* %stack.3 to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %15, i8* %16, i64 32, i1 false)
  %17 = load i32, i32* %seg_cnt
  ret i32 %17
}


define i32 @build([2000 x %SegT]* noalias %seg_pool, i32* noalias %seg_cnt, i32 %l, i32 %r) {
bb.entry:
  %stack.0 = alloca i32
  %stack.1 = alloca i32
  %stack.2 = alloca %Food
  %stack.3 = alloca %Food
  store i32 %l, i32* %stack.0
  store i32 %r, i32* %stack.1
  %0 = load i32, i32* %stack.0
  %1 = load i32, i32* %stack.1
  %2 = icmp sgt i32 %0, %1
  br i1 %2, label %if.then.3, label %if.end.3
if.then.3:
  ret i32 0
if.end.3:
  %3 = load i32, i32* %stack.0
  %4 = load i32, i32* %stack.1
  %5 = icmp eq i32 %3, %4
  br i1 %5, label %if.then.4, label %if.end.4
if.then.4:
  %6 = load i32, i32* %stack.0
  %7 = load i32, i32* %stack.1
  %8 = getelementptr inbounds %Food, %Food* %stack.2, i32 0, i32 0
  store i32 0, i32* %8
  %9 = getelementptr inbounds %Food, %Food* %stack.2, i32 0, i32 1
  %10 = load i32, i32* %stack.0
  store i32 %10, i32* %9
  %11 = call i32 @new_segt([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %6, i32 %7, %Food* %stack.2)
  ret i32 %11
if.end.4:
  %12 = load i32, i32* %stack.0
  %13 = load i32, i32* %stack.1
  %14 = getelementptr inbounds %Food, %Food* %stack.3, i32 0, i32 0
  store i32 0, i32* %14
  %15 = getelementptr inbounds %Food, %Food* %stack.3, i32 0, i32 1
  store i32 0, i32* %15
  %16 = call i32 @new_segt([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %12, i32 %13, %Food* %stack.3)
  ret i32 %16
}


define void @update([2000 x %SegT]* noalias %seg_pool, i32* noalias %seg_cnt, i32 %idx, i32 %pos, i32 %delta) {
bb.entry:
  %stack.0 = alloca i32
  %stack.1 = alloca i32
  %stack.2 = alloca i32
  %stack.3 = alloca i32
  %stack.4 = alloca i32
  %stack.5 = alloca i32
  %stack.6 = alloca %Food
  %stack.7 = alloca %Food
  %stack.8 = alloca %Food
  %stack.9 = alloca %Food
  %stack.10 = alloca %Food
  store i32 %idx, i32* %stack.0
  store i32 %pos, i32* %stack.1
  store i32 %delta, i32* %stack.2
  %0 = load i32, i32* %stack.0
  %1 = icmp eq i32 %0, 0
  br i1 %1, label %if.then.5, label %if.end.5
if.then.5:
  ret void
if.end.5:
  %2 = load i32, i32* %stack.0
  %3 = zext i32 %2 to i64
  %4 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %3
  %5 = getelementptr inbounds %SegT, %SegT* %4, i32 0, i32 0
  %6 = load i32, i32* %5
  store i32 %6, i32* %stack.3
  %7 = load i32, i32* %stack.0
  %8 = zext i32 %7 to i64
  %9 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %8
  %10 = getelementptr inbounds %SegT, %SegT* %9, i32 0, i32 1
  %11 = load i32, i32* %10
  store i32 %11, i32* %stack.4
  %12 = load i32, i32* %stack.1
  %13 = load i32, i32* %stack.3
  %14 = icmp slt i32 %12, %13
  br i1 %14, label %or.end.0, label %or.rhs.0
or.rhs.0:
  %15 = load i32, i32* %stack.1
  %16 = load i32, i32* %stack.4
  %17 = icmp sgt i32 %15, %16
  br label %or.end.0
or.end.0:
  %18 = phi i1 [%14, %if.end.5], [%17, %or.rhs.0]
  br i1 %18, label %if.then.6, label %if.end.6
if.then.6:
  ret void
if.end.6:
  %19 = load i32, i32* %stack.3
  %20 = load i32, i32* %stack.4
  %21 = icmp eq i32 %19, %20
  br i1 %21, label %if.then.7, label %if.end.7
if.then.7:
  %22 = load i32, i32* %stack.0
  %23 = zext i32 %22 to i64
  %24 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %23
  %25 = getelementptr inbounds %SegT, %SegT* %24, i32 0, i32 4
  %26 = getelementptr inbounds %Food, %Food* %25, i32 0, i32 0
  %27 = load i32, i32* %26
  %28 = load i32, i32* %stack.2
  %29 = add i32 %27, %28
  store i32 %29, i32* %26
  ret void
if.end.7:
  %30 = load i32, i32* %stack.3
  %31 = load i32, i32* %stack.4
  %32 = add i32 %30, %31
  %33 = sdiv i32 %32, 2
  store i32 %33, i32* %stack.5
  %34 = load i32, i32* %stack.1
  %35 = load i32, i32* %stack.5
  %36 = icmp sle i32 %34, %35
  br i1 %36, label %if.then.8, label %if.else.8
if.then.8:
  %37 = load i32, i32* %stack.0
  %38 = zext i32 %37 to i64
  %39 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %38
  %40 = getelementptr inbounds %SegT, %SegT* %39, i32 0, i32 2
  %41 = load i32, i32* %40
  %42 = icmp eq i32 %41, 0
  br i1 %42, label %if.then.9, label %if.end.9
if.then.9:
  %43 = load i32, i32* %stack.3
  %44 = load i32, i32* %stack.5
  %45 = call i32 @build([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %43, i32 %44)
  %46 = load i32, i32* %stack.0
  %47 = zext i32 %46 to i64
  %48 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %47
  %49 = getelementptr inbounds %SegT, %SegT* %48, i32 0, i32 2
  store i32 %45, i32* %49
  br label %if.end.9
if.end.9:
  %50 = load i32, i32* %stack.0
  %51 = zext i32 %50 to i64
  %52 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %51
  %53 = getelementptr inbounds %SegT, %SegT* %52, i32 0, i32 2
  %54 = load i32, i32* %53
  %55 = load i32, i32* %stack.1
  %56 = load i32, i32* %stack.2
  call void @update([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %54, i32 %55, i32 %56)
  br label %if.end.8
if.else.8:
  %57 = load i32, i32* %stack.0
  %58 = zext i32 %57 to i64
  %59 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %58
  %60 = getelementptr inbounds %SegT, %SegT* %59, i32 0, i32 3
  %61 = load i32, i32* %60
  %62 = icmp eq i32 %61, 0
  br i1 %62, label %if.then.10, label %if.end.10
if.then.10:
  %63 = load i32, i32* %stack.5
  %64 = add i32 %63, 1
  %65 = load i32, i32* %stack.4
  %66 = call i32 @build([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %64, i32 %65)
  %67 = load i32, i32* %stack.0
  %68 = zext i32 %67 to i64
  %69 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %68
  %70 = getelementptr inbounds %SegT, %SegT* %69, i32 0, i32 3
  store i32 %66, i32* %70
  br label %if.end.10
if.end.10:
  %71 = load i32, i32* %stack.0
  %72 = zext i32 %71 to i64
  %73 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %72
  %74 = getelementptr inbounds %SegT, %SegT* %73, i32 0, i32 3
  %75 = load i32, i32* %74
  %76 = load i32, i32* %stack.1
  %77 = load i32, i32* %stack.2
  call void @update([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %75, i32 %76, i32 %77)
  br label %if.end.8
if.end.8:
  %78 = getelementptr inbounds %Food, %Food* %stack.6, i32 0, i32 0
  store i32 0, i32* %78
  %79 = getelementptr inbounds %Food, %Food* %stack.6, i32 0, i32 1
  store i32 0, i32* %79
  %80 = load i32, i32* %stack.0
  %81 = zext i32 %80 to i64
  %82 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %81
  %83 = getelementptr inbounds %SegT, %SegT* %82, i32 0, i32 2
  %84 = load i32, i32* %83
  %85 = icmp ne i32 %84, 0
  br i1 %85, label %if.then.11, label %if.end.11
if.then.11:
  %86 = load i32, i32* %stack.0
  %87 = zext i32 %86 to i64
  %88 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %87
  call void @SegT_lc_val(%Food* %stack.7, %SegT* %88, [2000 x %SegT]* %seg_pool)
  call void @Food_better(%Food* %stack.8, %Food* %stack.7, %Food* %stack.6)
  %89 = bitcast %Food* %stack.6 to i8*
  %90 = bitcast %Food* %stack.8 to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %89, i8* %90, i64 8, i1 false)
  br label %if.end.11
if.end.11:
  %91 = load i32, i32* %stack.0
  %92 = zext i32 %91 to i64
  %93 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %92
  %94 = getelementptr inbounds %SegT, %SegT* %93, i32 0, i32 3
  %95 = load i32, i32* %94
  %96 = icmp ne i32 %95, 0
  br i1 %96, label %if.then.12, label %if.end.12
if.then.12:
  %97 = load i32, i32* %stack.0
  %98 = zext i32 %97 to i64
  %99 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %98
  call void @SegT_rc_val(%Food* %stack.9, %SegT* %99, [2000 x %SegT]* %seg_pool)
  call void @Food_better(%Food* %stack.10, %Food* %stack.9, %Food* %stack.6)
  %100 = bitcast %Food* %stack.6 to i8*
  %101 = bitcast %Food* %stack.10 to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %100, i8* %101, i64 8, i1 false)
  br label %if.end.12
if.end.12:
  %102 = load i32, i32* %stack.0
  %103 = zext i32 %102 to i64
  %104 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %103
  %105 = getelementptr inbounds %SegT, %SegT* %104, i32 0, i32 4
  %106 = bitcast %Food* %105 to i8*
  %107 = bitcast %Food* %stack.6 to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %106, i8* %107, i64 8, i1 false)
  ret void
}


define void @SegT_lc_val(%Food* %sret_ptr, %SegT* %self, [2000 x %SegT]* %seg_pool) {
bb.entry:
  %0 = getelementptr inbounds %SegT, %SegT* %self, i32 0, i32 2
  %1 = load i32, i32* %0
  %2 = zext i32 %1 to i64
  %3 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %2
  %4 = getelementptr inbounds %SegT, %SegT* %3, i32 0, i32 4
  ret void
}


define void @SegT_rc_val(%Food* %sret_ptr, %SegT* %self, [2000 x %SegT]* %seg_pool) {
bb.entry:
  %0 = getelementptr inbounds %SegT, %SegT* %self, i32 0, i32 3
  %1 = load i32, i32* %0
  %2 = zext i32 %1 to i64
  %3 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %2
  %4 = getelementptr inbounds %SegT, %SegT* %3, i32 0, i32 4
  ret void
}


define i32 @merge([2000 x %SegT]* noalias %seg_pool, i32* noalias %seg_cnt, i32 %u, i32 %v) {
bb.entry:
  %stack.0 = alloca i32
  %stack.1 = alloca i32
  %stack.2 = alloca %Food
  %stack.3 = alloca %Food
  %stack.4 = alloca %Food
  %stack.5 = alloca %Food
  %stack.6 = alloca %Food
  store i32 %u, i32* %stack.0
  store i32 %v, i32* %stack.1
  %0 = load i32, i32* %stack.0
  %1 = icmp eq i32 %0, 0
  br i1 %1, label %if.then.13, label %if.end.13
if.then.13:
  %2 = load i32, i32* %stack.1
  ret i32 %2
if.end.13:
  %3 = load i32, i32* %stack.1
  %4 = icmp eq i32 %3, 0
  br i1 %4, label %if.then.14, label %if.end.14
if.then.14:
  %5 = load i32, i32* %stack.0
  ret i32 %5
if.end.14:
  %6 = load i32, i32* %stack.0
  %7 = zext i32 %6 to i64
  %8 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %7
  %9 = getelementptr inbounds %SegT, %SegT* %8, i32 0, i32 0
  %10 = load i32, i32* %9
  %11 = load i32, i32* %stack.0
  %12 = zext i32 %11 to i64
  %13 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %12
  %14 = getelementptr inbounds %SegT, %SegT* %13, i32 0, i32 1
  %15 = load i32, i32* %14
  %16 = icmp eq i32 %10, %15
  br i1 %16, label %if.then.15, label %if.end.15
if.then.15:
  %17 = load i32, i32* %stack.0
  %18 = zext i32 %17 to i64
  %19 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %18
  %20 = getelementptr inbounds %SegT, %SegT* %19, i32 0, i32 4
  %21 = getelementptr inbounds %Food, %Food* %20, i32 0, i32 0
  %22 = load i32, i32* %21
  %23 = load i32, i32* %stack.1
  %24 = zext i32 %23 to i64
  %25 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %24
  %26 = getelementptr inbounds %SegT, %SegT* %25, i32 0, i32 4
  %27 = getelementptr inbounds %Food, %Food* %26, i32 0, i32 0
  %28 = load i32, i32* %27
  %29 = add i32 %22, %28
  store i32 %29, i32* %21
  %30 = load i32, i32* %stack.0
  ret i32 %30
if.end.15:
  %31 = load i32, i32* %stack.0
  %32 = zext i32 %31 to i64
  %33 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %32
  %34 = getelementptr inbounds %SegT, %SegT* %33, i32 0, i32 2
  %35 = load i32, i32* %34
  %36 = load i32, i32* %stack.1
  %37 = zext i32 %36 to i64
  %38 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %37
  %39 = getelementptr inbounds %SegT, %SegT* %38, i32 0, i32 2
  %40 = load i32, i32* %39
  %41 = call i32 @merge([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %35, i32 %40)
  %42 = load i32, i32* %stack.0
  %43 = zext i32 %42 to i64
  %44 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %43
  %45 = getelementptr inbounds %SegT, %SegT* %44, i32 0, i32 2
  store i32 %41, i32* %45
  %46 = load i32, i32* %stack.0
  %47 = zext i32 %46 to i64
  %48 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %47
  %49 = getelementptr inbounds %SegT, %SegT* %48, i32 0, i32 3
  %50 = load i32, i32* %49
  %51 = load i32, i32* %stack.1
  %52 = zext i32 %51 to i64
  %53 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %52
  %54 = getelementptr inbounds %SegT, %SegT* %53, i32 0, i32 3
  %55 = load i32, i32* %54
  %56 = call i32 @merge([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %50, i32 %55)
  %57 = load i32, i32* %stack.0
  %58 = zext i32 %57 to i64
  %59 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %58
  %60 = getelementptr inbounds %SegT, %SegT* %59, i32 0, i32 3
  store i32 %56, i32* %60
  %61 = getelementptr inbounds %Food, %Food* %stack.2, i32 0, i32 0
  store i32 0, i32* %61
  %62 = getelementptr inbounds %Food, %Food* %stack.2, i32 0, i32 1
  store i32 0, i32* %62
  %63 = load i32, i32* %stack.0
  %64 = zext i32 %63 to i64
  %65 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %64
  %66 = getelementptr inbounds %SegT, %SegT* %65, i32 0, i32 2
  %67 = load i32, i32* %66
  %68 = icmp ne i32 %67, 0
  br i1 %68, label %if.then.16, label %if.end.16
if.then.16:
  %69 = load i32, i32* %stack.0
  %70 = zext i32 %69 to i64
  %71 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %70
  call void @SegT_lc_val(%Food* %stack.3, %SegT* %71, [2000 x %SegT]* %seg_pool)
  call void @Food_better(%Food* %stack.4, %Food* %stack.3, %Food* %stack.2)
  %72 = bitcast %Food* %stack.2 to i8*
  %73 = bitcast %Food* %stack.4 to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %72, i8* %73, i64 8, i1 false)
  br label %if.end.16
if.end.16:
  %74 = load i32, i32* %stack.0
  %75 = zext i32 %74 to i64
  %76 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %75
  %77 = getelementptr inbounds %SegT, %SegT* %76, i32 0, i32 3
  %78 = load i32, i32* %77
  %79 = icmp ne i32 %78, 0
  br i1 %79, label %if.then.17, label %if.end.17
if.then.17:
  %80 = load i32, i32* %stack.0
  %81 = zext i32 %80 to i64
  %82 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %81
  call void @SegT_rc_val(%Food* %stack.5, %SegT* %82, [2000 x %SegT]* %seg_pool)
  call void @Food_better(%Food* %stack.6, %Food* %stack.5, %Food* %stack.2)
  %83 = bitcast %Food* %stack.2 to i8*
  %84 = bitcast %Food* %stack.6 to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %83, i8* %84, i64 8, i1 false)
  br label %if.end.17
if.end.17:
  %85 = load i32, i32* %stack.0
  %86 = zext i32 %85 to i64
  %87 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %86
  %88 = getelementptr inbounds %SegT, %SegT* %87, i32 0, i32 4
  %89 = bitcast %Food* %88 to i8*
  %90 = bitcast %Food* %stack.2 to i8*
  call void @llvm.memcpy.p0.p0.i64(i8* %89, i8* %90, i64 8, i1 false)
  %91 = load i32, i32* %stack.0
  ret i32 %91
}


define void @Node_push(%Node* noalias %self, i32 %to) {
bb.entry:
  %stack.0 = alloca i32
  store i32 %to, i32* %stack.0
  %0 = load i32, i32* %stack.0
  %1 = getelementptr inbounds %Node, %Node* %self, i32 0, i32 0
  %2 = getelementptr inbounds %Node, %Node* %self, i32 0, i32 1
  %3 = load i32, i32* %2
  %4 = zext i32 %3 to i64
  %5 = getelementptr inbounds [201 x i32], [201 x i32]* %1, i64 0, i64 %4
  store i32 %0, i32* %5
  %6 = getelementptr inbounds %Node, %Node* %self, i32 0, i32 1
  %7 = load i32, i32* %6
  %8 = add i32 %7, 1
  store i32 %8, i32* %6
  ret void
}


define void @Node_clear(%Node* noalias %self) {
bb.entry:
  %0 = getelementptr inbounds %Node, %Node* %self, i32 0, i32 1
  store i32 0, i32* %0
  ret void
}


define void @prepare([201 x %Node]* noalias %node_pool, i32 %v, i32 %f) {
bb.entry:
  %stack.0 = alloca i32
  %stack.1 = alloca i32
  %stack.2 = alloca i32
  %stack.3 = alloca i32
  %stack.4 = alloca i32
  store i32 %v, i32* %stack.0
  store i32 %f, i32* %stack.1
  %0 = load i32, i32* %stack.1
  %1 = load i32, i32* %stack.0
  %2 = zext i32 %1 to i64
  %3 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %2
  %4 = getelementptr inbounds %Node, %Node* %3, i32 0, i32 3
  %5 = zext i32 0 to i64
  %6 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 %5
  store i32 %0, i32* %6
  %7 = load i32, i32* %stack.1
  %8 = icmp eq i32 %7, 0
  br i1 %8, label %if.then.18, label %if.else.18
if.then.18:
  br label %if.end.18
if.else.18:
  %9 = load i32, i32* %stack.1
  %10 = zext i32 %9 to i64
  %11 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %10
  %12 = getelementptr inbounds %Node, %Node* %11, i32 0, i32 2
  %13 = load i32, i32* %12
  %14 = add i32 %13, 1
  br label %if.end.18
if.end.18:
  %15 = phi i32 [1, %if.then.18], [%14, %if.else.18]
  %16 = load i32, i32* %stack.0
  %17 = zext i32 %16 to i64
  %18 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %17
  %19 = getelementptr inbounds %Node, %Node* %18, i32 0, i32 2
  store i32 %15, i32* %19
  store i32 1, i32* %stack.2
  br label %while.cond.0
while.cond.0:
  %20 = load i32, i32* %stack.2
  %21 = load i32, i32* @LOG_MAXN
  %22 = icmp ule i32 %20, %21
  br i1 %22, label %while.body.0, label %while.end.0
while.body.0:
  %23 = load i32, i32* %stack.0
  %24 = zext i32 %23 to i64
  %25 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %24
  %26 = getelementptr inbounds %Node, %Node* %25, i32 0, i32 3
  %27 = load i32, i32* %stack.2
  %28 = sub i32 %27, 1
  %29 = zext i32 %28 to i64
  %30 = getelementptr inbounds [11 x i32], [11 x i32]* %26, i64 0, i64 %29
  %31 = load i32, i32* %30
  store i32 %31, i32* %stack.3
  %32 = load i32, i32* %stack.3
  %33 = icmp ne i32 %32, 0
  br i1 %33, label %if.then.19, label %if.end.19
if.then.19:
  %34 = load i32, i32* %stack.3
  %35 = zext i32 %34 to i64
  %36 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %35
  %37 = getelementptr inbounds %Node, %Node* %36, i32 0, i32 3
  %38 = load i32, i32* %stack.2
  %39 = sub i32 %38, 1
  %40 = zext i32 %39 to i64
  %41 = getelementptr inbounds [11 x i32], [11 x i32]* %37, i64 0, i64 %40
  %42 = load i32, i32* %41
  %43 = load i32, i32* %stack.0
  %44 = zext i32 %43 to i64
  %45 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %44
  %46 = getelementptr inbounds %Node, %Node* %45, i32 0, i32 3
  %47 = load i32, i32* %stack.2
  %48 = zext i32 %47 to i64
  %49 = getelementptr inbounds [11 x i32], [11 x i32]* %46, i64 0, i64 %48
  store i32 %42, i32* %49
  br label %if.end.19
if.end.19:
  %50 = load i32, i32* %stack.2
  %51 = add i32 %50, 1
  store i32 %51, i32* %stack.2
  br label %while.cond.0
while.end.0:
  store i32 0, i32* %stack.2
  br label %while.cond.1
while.cond.1:
  %52 = load i32, i32* %stack.2
  %53 = load i32, i32* %stack.0
  %54 = zext i32 %53 to i64
  %55 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %54
  %56 = getelementptr inbounds %Node, %Node* %55, i32 0, i32 1
  %57 = load i32, i32* %56
  %58 = icmp ult i32 %52, %57
  br i1 %58, label %while.body.1, label %while.end.1
while.body.1:
  %59 = load i32, i32* %stack.0
  %60 = zext i32 %59 to i64
  %61 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %60
  %62 = getelementptr inbounds %Node, %Node* %61, i32 0, i32 0
  %63 = load i32, i32* %stack.2
  %64 = zext i32 %63 to i64
  %65 = getelementptr inbounds [201 x i32], [201 x i32]* %62, i64 0, i64 %64
  %66 = load i32, i32* %65
  store i32 %66, i32* %stack.4
  %67 = load i32, i32* %stack.2
  %68 = add i32 %67, 1
  store i32 %68, i32* %stack.2
  %69 = load i32, i32* %stack.4
  %70 = load i32, i32* %stack.1
  %71 = icmp eq i32 %69, %70
  br i1 %71, label %if.then.20, label %if.end.20
if.then.20:
  br label %while.cond.1
if.end.20:
  %72 = load i32, i32* %stack.4
  %73 = load i32, i32* %stack.0
  call void @prepare([201 x %Node]* %node_pool, i32 %72, i32 %73)
  br label %while.cond.1
while.end.1:
  ret void
}


define i32 @lca([201 x %Node]* %node_pool, i32 %u, i32 %v) {
bb.entry:
  %stack.0 = alloca i32
  %stack.1 = alloca i32
  %stack.2 = alloca i32
  %stack.3 = alloca i32
  %stack.4 = alloca i32
  store i32 %u, i32* %stack.0
  store i32 %v, i32* %stack.1
  %0 = load i32, i32* %stack.0
  %1 = zext i32 %0 to i64
  %2 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %1
  %3 = getelementptr inbounds %Node, %Node* %2, i32 0, i32 2
  %4 = load i32, i32* %3
  %5 = load i32, i32* %stack.1
  %6 = zext i32 %5 to i64
  %7 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %6
  %8 = getelementptr inbounds %Node, %Node* %7, i32 0, i32 2
  %9 = load i32, i32* %8
  %10 = icmp slt i32 %4, %9
  br i1 %10, label %if.then.21, label %if.end.21
if.then.21:
  %11 = load i32, i32* %stack.0
  store i32 %11, i32* %stack.2
  %12 = load i32, i32* %stack.1
  store i32 %12, i32* %stack.0
  %13 = load i32, i32* %stack.2
  store i32 %13, i32* %stack.1
  br label %if.end.21
if.end.21:
  %14 = load i32, i32* @LOG_MAXN
  store i32 %14, i32* %stack.3
  br label %while.cond.2
while.cond.2:
  %15 = load i32, i32* %stack.3
  %16 = icmp sge i32 %15, 0
  br i1 %16, label %while.body.2, label %while.end.2
while.body.2:
  %17 = load i32, i32* %stack.0
  %18 = zext i32 %17 to i64
  %19 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %18
  %20 = getelementptr inbounds %Node, %Node* %19, i32 0, i32 3
  %21 = load i32, i32* %stack.3
  %22 = zext i32 %21 to i64
  %23 = getelementptr inbounds [11 x i32], [11 x i32]* %20, i64 0, i64 %22
  %24 = load i32, i32* %23
  store i32 %24, i32* %stack.4
  %25 = load i32, i32* %stack.4
  %26 = icmp ne i32 %25, 0
  br i1 %26, label %and.rhs.1, label %and.end.1
and.rhs.1:
  %27 = load i32, i32* %stack.4
  %28 = zext i32 %27 to i64
  %29 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %28
  %30 = getelementptr inbounds %Node, %Node* %29, i32 0, i32 2
  %31 = load i32, i32* %30
  %32 = load i32, i32* %stack.1
  %33 = zext i32 %32 to i64
  %34 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %33
  %35 = getelementptr inbounds %Node, %Node* %34, i32 0, i32 2
  %36 = load i32, i32* %35
  %37 = icmp sge i32 %31, %36
  br label %and.end.1
and.end.1:
  %38 = phi i1 [%26, %while.body.2], [%37, %and.rhs.1]
  br i1 %38, label %if.then.22, label %if.end.22
if.then.22:
  %39 = load i32, i32* %stack.4
  store i32 %39, i32* %stack.0
  br label %if.end.22
if.end.22:
  %40 = load i32, i32* %stack.3
  %41 = sub i32 %40, 1
  store i32 %41, i32* %stack.3
  br label %while.cond.2
while.end.2:
  %42 = load i32, i32* %stack.0
  %43 = load i32, i32* %stack.1
  %44 = icmp eq i32 %42, %43
  br i1 %44, label %if.then.23, label %if.end.23
if.then.23:
  %45 = load i32, i32* %stack.0
  ret i32 %45
if.end.23:
  %46 = load i32, i32* @LOG_MAXN
  store i32 %46, i32* %stack.3
  br label %while.cond.3
while.cond.3:
  %47 = load i32, i32* %stack.3
  %48 = icmp sge i32 %47, 0
  br i1 %48, label %while.body.3, label %while.end.3
while.body.3:
  %49 = load i32, i32* %stack.0
  %50 = zext i32 %49 to i64
  %51 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %50
  %52 = getelementptr inbounds %Node, %Node* %51, i32 0, i32 3
  %53 = load i32, i32* %stack.3
  %54 = zext i32 %53 to i64
  %55 = getelementptr inbounds [11 x i32], [11 x i32]* %52, i64 0, i64 %54
  %56 = load i32, i32* %55
  %57 = load i32, i32* %stack.1
  %58 = zext i32 %57 to i64
  %59 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %58
  %60 = getelementptr inbounds %Node, %Node* %59, i32 0, i32 3
  %61 = load i32, i32* %stack.3
  %62 = zext i32 %61 to i64
  %63 = getelementptr inbounds [11 x i32], [11 x i32]* %60, i64 0, i64 %62
  %64 = load i32, i32* %63
  %65 = icmp ne i32 %56, %64
  br i1 %65, label %if.then.24, label %if.end.24
if.then.24:
  %66 = load i32, i32* %stack.0
  %67 = zext i32 %66 to i64
  %68 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %67
  %69 = getelementptr inbounds %Node, %Node* %68, i32 0, i32 3
  %70 = load i32, i32* %stack.3
  %71 = zext i32 %70 to i64
  %72 = getelementptr inbounds [11 x i32], [11 x i32]* %69, i64 0, i64 %71
  %73 = load i32, i32* %72
  store i32 %73, i32* %stack.0
  %74 = load i32, i32* %stack.1
  %75 = zext i32 %74 to i64
  %76 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %75
  %77 = getelementptr inbounds %Node, %Node* %76, i32 0, i32 3
  %78 = load i32, i32* %stack.3
  %79 = zext i32 %78 to i64
  %80 = getelementptr inbounds [11 x i32], [11 x i32]* %77, i64 0, i64 %79
  %81 = load i32, i32* %80
  store i32 %81, i32* %stack.1
  br label %if.end.24
if.end.24:
  %82 = load i32, i32* %stack.3
  %83 = sub i32 %82, 1
  store i32 %83, i32* %stack.3
  br label %while.cond.3
while.end.3:
  %84 = load i32, i32* %stack.0
  %85 = zext i32 %84 to i64
  %86 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %85
  %87 = getelementptr inbounds %Node, %Node* %86, i32 0, i32 3
  %88 = zext i32 0 to i64
  %89 = getelementptr inbounds [11 x i32], [11 x i32]* %87, i64 0, i64 %88
  %90 = load i32, i32* %89
  ret i32 %90
}


define void @dfs([201 x %Node]* noalias %node_pool, [2000 x %SegT]* noalias %seg_pool, i32* noalias %seg_cnt, i32 %v, i32 %f) {
bb.entry:
  %stack.0 = alloca i32
  %stack.1 = alloca i32
  %stack.2 = alloca i32
  %stack.3 = alloca i32
  store i32 %v, i32* %stack.0
  store i32 %f, i32* %stack.1
  store i32 0, i32* %stack.2
  br label %while.cond.4
while.cond.4:
  %0 = load i32, i32* %stack.2
  %1 = load i32, i32* %stack.0
  %2 = zext i32 %1 to i64
  %3 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %2
  %4 = getelementptr inbounds %Node, %Node* %3, i32 0, i32 1
  %5 = load i32, i32* %4
  %6 = icmp ult i32 %0, %5
  br i1 %6, label %while.body.4, label %while.end.4
while.body.4:
  %7 = load i32, i32* %stack.0
  %8 = zext i32 %7 to i64
  %9 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %8
  %10 = getelementptr inbounds %Node, %Node* %9, i32 0, i32 0
  %11 = load i32, i32* %stack.2
  %12 = zext i32 %11 to i64
  %13 = getelementptr inbounds [201 x i32], [201 x i32]* %10, i64 0, i64 %12
  %14 = load i32, i32* %13
  store i32 %14, i32* %stack.3
  %15 = load i32, i32* %stack.2
  %16 = add i32 %15, 1
  store i32 %16, i32* %stack.2
  %17 = load i32, i32* %stack.3
  %18 = load i32, i32* %stack.1
  %19 = icmp eq i32 %17, %18
  br i1 %19, label %if.then.25, label %if.end.25
if.then.25:
  br label %while.cond.4
if.end.25:
  %20 = load i32, i32* %stack.3
  %21 = load i32, i32* %stack.0
  call void @dfs([201 x %Node]* %node_pool, [2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %20, i32 %21)
  br label %while.cond.4
while.end.4:
  %22 = load i32, i32* %stack.0
  %23 = zext i32 %22 to i64
  %24 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %23
  %25 = getelementptr inbounds %Node, %Node* %24, i32 0, i32 4
  %26 = load i32, i32* %25
  %27 = zext i32 %26 to i64
  %28 = getelementptr inbounds [2000 x %SegT], [2000 x %SegT]* %seg_pool, i64 0, i64 %27
  %29 = getelementptr inbounds %SegT, %SegT* %28, i32 0, i32 4
  %30 = getelementptr inbounds %Food, %Food* %29, i32 0, i32 1
  %31 = load i32, i32* %30
  %32 = load i32, i32* %stack.0
  %33 = zext i32 %32 to i64
  %34 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %33
  %35 = getelementptr inbounds %Node, %Node* %34, i32 0, i32 5
  store i32 %31, i32* %35
  %36 = load i32, i32* %stack.1
  %37 = icmp ne i32 %36, 0
  br i1 %37, label %if.then.26, label %if.end.26
if.then.26:
  %38 = load i32, i32* %stack.1
  %39 = zext i32 %38 to i64
  %40 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %39
  %41 = getelementptr inbounds %Node, %Node* %40, i32 0, i32 4
  %42 = load i32, i32* %41
  %43 = load i32, i32* %stack.0
  %44 = zext i32 %43 to i64
  %45 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %44
  %46 = getelementptr inbounds %Node, %Node* %45, i32 0, i32 4
  %47 = load i32, i32* %46
  %48 = call i32 @merge([2000 x %SegT]* %seg_pool, i32* %seg_cnt, i32 %42, i32 %47)
  %49 = load i32, i32* %stack.1
  %50 = zext i32 %49 to i64
  %51 = getelementptr inbounds [201 x %Node], [201 x %Node]* %node_pool, i64 0, i64 %50
  %52 = getelementptr inbounds %Node, %Node* %51, i32 0, i32 4
  store i32 %48, i32* %52
  br label %if.end.26
if.end.26:
  ret void
}


define void @main() {
bb.entry:
  %stack.0 = alloca %Node
  %stack.1 = alloca [201 x %Node]
  %stack.2 = alloca i64
  %stack.3 = alloca %SegT
  %stack.4 = alloca [2000 x %SegT]
  %stack.5 = alloca i32
  %stack.6 = alloca i32
  %stack.7 = alloca i32
  %stack.8 = alloca i32
  %stack.9 = alloca i32
  %stack.10 = alloca i32
  %stack.11 = alloca i32
  %stack.12 = alloca i32
  %stack.13 = alloca i32
  %stack.14 = alloca i32
  %stack.15 = alloca i32
  %stack.16 = alloca i32
  %stack.17 = alloca i32
  %stack.18 = alloca i32
  %stack.19 = alloca i32
  %stack.20 = alloca i32
  %stack.21 = alloca i32
  %0 = getelementptr inbounds %Node, %Node* %stack.0, i32 0, i32 0
  %1 = bitcast [201 x i32]* %0 to i8*
  call void @llvm.memset.p0.i64(i8* %1, i8 0, i64 804, i1 false)
  %2 = getelementptr inbounds %Node, %Node* %stack.0, i32 0, i32 1
  store i32 0, i32* %2
  %3 = getelementptr inbounds %Node, %Node* %stack.0, i32 0, i32 2
  store i32 0, i32* %3
  %4 = getelementptr inbounds %Node, %Node* %stack.0, i32 0, i32 3
  %5 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 0
  store i32 0, i32* %5
  %6 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 1
  store i32 0, i32* %6
  %7 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 2
  store i32 0, i32* %7
  %8 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 3
  store i32 0, i32* %8
  %9 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 4
  store i32 0, i32* %9
  %10 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 5
  store i32 0, i32* %10
  %11 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 6
  store i32 0, i32* %11
  %12 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 7
  store i32 0, i32* %12
  %13 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 8
  store i32 0, i32* %13
  %14 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 9
  store i32 0, i32* %14
  %15 = getelementptr inbounds [11 x i32], [11 x i32]* %4, i64 0, i64 10
  store i32 0, i32* %15
  %16 = getelementptr inbounds %Node, %Node* %stack.0, i32 0, i32 4
  store i32 0, i32* %16
  %17 = getelementptr inbounds %Node, %Node* %stack.0, i32 0, i32 5
  store i32 0, i32* %17
  %18 = load %Node, %Node* %stack.0
  store i64 0, i64* %stack.2
  br label %label0
label0:
  %19 = load i64, i64* %stack.2
  %20 = icmp slt i64 %19, 201
  br i1 %20, label %label1, label %label2
label1:
  %21 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %19
  store %Node %18, %Node* %21
  %22 = add i64 %19, 1
  store i64 %22, i64* %stack.2
  br label %label0
label2:
  %23 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 0
  store i32 0, i32* %23
  %24 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 1
  store i32 0, i32* %24
  %25 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 2
  store i32 0, i32* %25
  %26 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 3
  store i32 0, i32* %26
  %27 = getelementptr inbounds %SegT, %SegT* %stack.3, i32 0, i32 4
  %28 = getelementptr inbounds %Food, %Food* %27, i32 0, i32 0
  store i32 0, i32* %28
  %29 = getelementptr inbounds %Food, %Food* %27, i32 0, i32 1
  store i32 0, i32* %29
  %30 = load %SegT, %SegT* %stack.3
  %31 = bitcast [2000 x %SegT]* %stack.4 to i8*
  call void @llvm.memset.p0.i64(i8* %31, i8 0, i64 64000, i1 false)
  store i32 0, i32* %stack.5
  %32 = getelementptr [3 x i8], [3 x i8]* @.str.int_scanf, i32 0, i32 0
  %33 = call i32 (i8*, ...) @scanf(i8* %32, i32* %stack.7)
  %34 = load i32, i32* %stack.7
  store i32 %34, i32* %stack.6
  %35 = getelementptr [3 x i8], [3 x i8]* @.str.int_scanf, i32 0, i32 0
  %36 = call i32 (i8*, ...) @scanf(i8* %35, i32* %stack.9)
  %37 = load i32, i32* %stack.9
  store i32 %37, i32* %stack.8
  store i32 1, i32* %stack.10
  br label %while.cond.5
while.cond.5:
  %38 = load i32, i32* %stack.10
  %39 = load i32, i32* %stack.6
  %40 = icmp ule i32 %38, %39
  br i1 %40, label %while.body.5, label %while.end.5
while.body.5:
  %41 = load i32, i32* %stack.10
  %42 = zext i32 %41 to i64
  %43 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %42
  call void @Node_clear(%Node* %43)
  %44 = load i32, i32* @MAXN
  %45 = call i32 @build([2000 x %SegT]* %stack.4, i32* %stack.5, i32 1, i32 %44)
  %46 = load i32, i32* %stack.10
  %47 = zext i32 %46 to i64
  %48 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %47
  %49 = getelementptr inbounds %Node, %Node* %48, i32 0, i32 4
  store i32 %45, i32* %49
  %50 = load i32, i32* %stack.10
  %51 = add i32 %50, 1
  store i32 %51, i32* %stack.10
  br label %while.cond.5
while.end.5:
  store i32 0, i32* %stack.10
  br label %while.cond.6
while.cond.6:
  %52 = load i32, i32* %stack.10
  %53 = load i32, i32* %stack.6
  %54 = sub i32 %53, 1
  %55 = icmp ult i32 %52, %54
  br i1 %55, label %while.body.6, label %while.end.6
while.body.6:
  %56 = getelementptr [3 x i8], [3 x i8]* @.str.int_scanf, i32 0, i32 0
  %57 = call i32 (i8*, ...) @scanf(i8* %56, i32* %stack.12)
  %58 = load i32, i32* %stack.12
  store i32 %58, i32* %stack.11
  %59 = getelementptr [3 x i8], [3 x i8]* @.str.int_scanf, i32 0, i32 0
  %60 = call i32 (i8*, ...) @scanf(i8* %59, i32* %stack.14)
  %61 = load i32, i32* %stack.14
  store i32 %61, i32* %stack.13
  %62 = load i32, i32* %stack.13
  %63 = load i32, i32* %stack.11
  %64 = zext i32 %63 to i64
  %65 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %64
  call void @Node_push(%Node* %65, i32 %62)
  %66 = load i32, i32* %stack.11
  %67 = load i32, i32* %stack.13
  %68 = zext i32 %67 to i64
  %69 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %68
  call void @Node_push(%Node* %69, i32 %66)
  %70 = load i32, i32* %stack.10
  %71 = add i32 %70, 1
  store i32 %71, i32* %stack.10
  br label %while.cond.6
while.end.6:
  call void @prepare([201 x %Node]* %stack.1, i32 1, i32 0)
  store i32 0, i32* %stack.10
  br label %while.cond.7
while.cond.7:
  %72 = load i32, i32* %stack.10
  %73 = load i32, i32* %stack.8
  %74 = icmp ult i32 %72, %73
  br i1 %74, label %while.body.7, label %while.end.7
while.body.7:
  %75 = getelementptr [3 x i8], [3 x i8]* @.str.int_scanf, i32 0, i32 0
  %76 = call i32 (i8*, ...) @scanf(i8* %75, i32* %stack.16)
  %77 = load i32, i32* %stack.16
  store i32 %77, i32* %stack.15
  %78 = getelementptr [3 x i8], [3 x i8]* @.str.int_scanf, i32 0, i32 0
  %79 = call i32 (i8*, ...) @scanf(i8* %78, i32* %stack.18)
  %80 = load i32, i32* %stack.18
  store i32 %80, i32* %stack.17
  %81 = getelementptr [3 x i8], [3 x i8]* @.str.int_scanf, i32 0, i32 0
  %82 = call i32 (i8*, ...) @scanf(i8* %81, i32* %stack.20)
  %83 = load i32, i32* %stack.20
  store i32 %83, i32* %stack.19
  %84 = load i32, i32* %stack.15
  %85 = load i32, i32* %stack.17
  %86 = call i32 @lca([201 x %Node]* %stack.1, i32 %84, i32 %85)
  store i32 %86, i32* %stack.21
  %87 = load i32, i32* %stack.15
  %88 = zext i32 %87 to i64
  %89 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %88
  %90 = getelementptr inbounds %Node, %Node* %89, i32 0, i32 4
  %91 = load i32, i32* %90
  %92 = load i32, i32* %stack.19
  call void @update([2000 x %SegT]* %stack.4, i32* %stack.5, i32 %91, i32 %92, i32 1)
  %93 = load i32, i32* %stack.17
  %94 = zext i32 %93 to i64
  %95 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %94
  %96 = getelementptr inbounds %Node, %Node* %95, i32 0, i32 4
  %97 = load i32, i32* %96
  %98 = load i32, i32* %stack.19
  call void @update([2000 x %SegT]* %stack.4, i32* %stack.5, i32 %97, i32 %98, i32 1)
  %99 = load i32, i32* %stack.21
  %100 = zext i32 %99 to i64
  %101 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %100
  %102 = getelementptr inbounds %Node, %Node* %101, i32 0, i32 4
  %103 = load i32, i32* %102
  %104 = load i32, i32* %stack.19
  %105 = sub i32 0, 1
  call void @update([2000 x %SegT]* %stack.4, i32* %stack.5, i32 %103, i32 %104, i32 %105)
  %106 = load i32, i32* %stack.21
  %107 = zext i32 %106 to i64
  %108 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %107
  %109 = getelementptr inbounds %Node, %Node* %108, i32 0, i32 3
  %110 = zext i32 0 to i64
  %111 = getelementptr inbounds [11 x i32], [11 x i32]* %109, i64 0, i64 %110
  %112 = load i32, i32* %111
  %113 = icmp ne i32 %112, 0
  br i1 %113, label %if.then.27, label %if.end.27
if.then.27:
  %114 = load i32, i32* %stack.21
  %115 = zext i32 %114 to i64
  %116 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %115
  %117 = getelementptr inbounds %Node, %Node* %116, i32 0, i32 3
  %118 = zext i32 0 to i64
  %119 = getelementptr inbounds [11 x i32], [11 x i32]* %117, i64 0, i64 %118
  %120 = load i32, i32* %119
  %121 = zext i32 %120 to i64
  %122 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %121
  %123 = getelementptr inbounds %Node, %Node* %122, i32 0, i32 4
  %124 = load i32, i32* %123
  %125 = load i32, i32* %stack.19
  %126 = sub i32 0, 1
  call void @update([2000 x %SegT]* %stack.4, i32* %stack.5, i32 %124, i32 %125, i32 %126)
  br label %if.end.27
if.end.27:
  %127 = load i32, i32* %stack.10
  %128 = add i32 %127, 1
  store i32 %128, i32* %stack.10
  br label %while.cond.7
while.end.7:
  call void @dfs([201 x %Node]* %stack.1, [2000 x %SegT]* %stack.4, i32* %stack.5, i32 1, i32 0)
  store i32 1, i32* %stack.10
  br label %while.cond.8
while.cond.8:
  %129 = load i32, i32* %stack.10
  %130 = load i32, i32* %stack.6
  %131 = icmp ule i32 %129, %130
  br i1 %131, label %while.body.8, label %while.end.8
while.body.8:
  %132 = load i32, i32* %stack.10
  %133 = zext i32 %132 to i64
  %134 = getelementptr inbounds [201 x %Node], [201 x %Node]* %stack.1, i64 0, i64 %133
  %135 = getelementptr inbounds %Node, %Node* %134, i32 0, i32 5
  %136 = load i32, i32* %135
  %137 = getelementptr [4 x i8], [4 x i8]* @.str.int_newline, i32 0, i32 0
  %138 = call i32 (i8*, ...) @printf(i8* %137, i32 %136)
  %139 = load i32, i32* %stack.10
  %140 = add i32 %139, 1
  store i32 %140, i32* %stack.10
  br label %while.cond.8
while.end.8:
  call void @exit(i32 0)
  unreachable
}


