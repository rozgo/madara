/*********************************************************************
* Usage of this software requires acceptance of the SMASH-CMU License,
* which can be found at the following URL:
*
* https://code.google.com/p/smash-cmu/wiki/License
*********************************************************************/

#include "com.madara.Variables.h"
#include "madara/knowledge/KnowledgeBase.h"

// define useful shorthands
namespace knowledge = madara::knowledge;
typedef knowledge::Variables Variables;
typedef knowledge::CompiledExpression   CompiledExpression;
typedef knowledge::EvalSettings         EvalSettings;
typedef knowledge::KnowledgeRecord      KnowledgeRecord;

/*
* Class:     com_madara_Variables
* Method:    jni_evaluate
* Signature: (JJJ)J
*/
jlong JNICALL
Java_com_madara_Variables_jni_1evaluate__JJJ (JNIEnv * env, jobject obj, jlong cptr, jlong expression, jlong evalSettings)
{
  KnowledgeRecord * result (0);
  Variables * vars = (Variables *) cptr;
  CompiledExpression * compiled = (CompiledExpression*) expression;
  EvalSettings * settings = (EvalSettings*) evalSettings;

  if (vars && compiled && settings)
  {
    result = new KnowledgeRecord (vars->evaluate (*compiled, *settings));
  }

  return (jlong) result;
}

/*
* Class:     com_madara_Variables
* Method:    jni_compile
* Signature: (JLjava/lang/String;)J
*/
jlong JNICALL
Java_com_madara_Variables_jni_1compile (JNIEnv *env, jobject obj, jlong cptr, jstring expression)
{
  const char *nativeExpression = env->GetStringUTFChars (expression, 0);

  Variables * vars = (Variables *) cptr;
  CompiledExpression * result (0);

  if (vars)
  {
    result = new CompiledExpression (vars->compile (nativeExpression));

    env->ReleaseStringUTFChars (expression, nativeExpression);
  }

  return (jlong) result;
}

/*
* Class:     com_madara_Variables
* Method:    jni_get
* Signature: (JLjava/lang/String;)J
*/
jlong JNICALL
Java_com_madara_Variables_jni_1get (JNIEnv * env, jobject obj, jlong cptr, jstring var)
{
  const char * nativeVar = env->GetStringUTFChars (var, 0);

  Variables * vars = (Variables *) cptr;
  KnowledgeRecord * result (0);

  if (vars)
  {
    result = new KnowledgeRecord (vars->get (nativeVar));
  }

  env->ReleaseStringUTFChars (var, nativeVar);
  return (jlong) result;
}

/*
* Class:     com_madara_Variables
* Method:    jni_set
* Signature: (JLjava/lang/String;J)V
*/
void JNICALL
Java_com_madara_Variables_jni_1set (JNIEnv *env, jobject obj, jlong cptr, jstring var, jlong recordPtr)
{
  const char *nativeVar = env->GetStringUTFChars (var, 0);

  Variables * vars = (Variables *) cptr;
  KnowledgeRecord * record = (KnowledgeRecord*) recordPtr;

  if (vars)
  {
    vars->set (nativeVar, *record);
  }

  env->ReleaseStringUTFChars (var, nativeVar);
}
