/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
#include "madara/MADARA_export.h"
/* Header for class com_madara_containers_Counter */

#ifndef _Included_com_madara_containers_Counter
#define _Included_com_madara_containers_Counter
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_Counter
 * Signature: ()J
 */
MADARA_Export jlong JNICALL Java_com_madara_containers_Counter_jni_1Counter__
  (JNIEnv *, jobject);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_Counter
 * Signature: (J)J
 */
MADARA_Export jlong JNICALL Java_com_madara_containers_Counter_jni_1Counter__J
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_freeCounter
 * Signature: (J)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1freeCounter
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_set
 * Signature: (JJ)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1set
  (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_getName
 * Signature: (J)Ljava/lang/String;
 */
MADARA_Export jstring JNICALL Java_com_madara_containers_Counter_jni_1getName
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_setName
 * Signature: (JJJLjava/lang/String;)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1setName
  (JNIEnv *, jobject, jlong, jlong, jlong, jstring);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_toString
 * Signature: (J)Ljava/lang/String;
 */
MADARA_Export jstring JNICALL Java_com_madara_containers_Counter_jni_1toString
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_toDouble
 * Signature: (J)D
 */
MADARA_Export jdouble JNICALL Java_com_madara_containers_Counter_jni_1toDouble
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_toLong
 * Signature: (J)J
 */
MADARA_Export jlong JNICALL Java_com_madara_containers_Counter_jni_1toLong
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_inc
 * Signature: (J)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1inc
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_dec
 * Signature: (J)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1dec
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_incValue
 * Signature: (JJ)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1incValue
  (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_decValue
 * Signature: (JJ)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1decValue
  (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_modify
 * Signature: (J)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1modify
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_madara_containers_Counter
 * Method:    jni_resize
 * Signature: (JII)V
 */
MADARA_Export void JNICALL Java_com_madara_containers_Counter_jni_1resize
  (JNIEnv *, jobject, jlong, jint, jint);

#ifdef __cplusplus
}
#endif
#endif