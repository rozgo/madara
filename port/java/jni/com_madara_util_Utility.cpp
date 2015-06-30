#include "com_madara_util_Utility.h"
#include "madara/utility/Utility.h"

namespace utility = Madara::Utility;

jstring JNICALL Java_com_madara_util_Utility_jni_1getVersion
(JNIEnv * env, jclass)
{
  return env->NewStringUTF (utility::get_version ().c_str ());
}

jlong JNICALL Java_com_madara_util_Utility_jni_1getTime
(JNIEnv *, jclass)
{
  return (jlong) utility::get_time ();
}

void JNICALL Java_com_madara_util_Utility_jni_1sleep
(JNIEnv *, jclass, jdouble sleep_time)
{
  utility::sleep (sleep_time);
}
