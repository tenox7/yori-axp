/**
 * @file builtins/fg.c
 *
 * Yori shell display job output in the foreground
 *
 * Copyright (c) 2017-2018 Malcolm J. Smith
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <yoripch.h>
#include <yorilib.h>
#include <yoricall.h>

/**
 Help text to display to the user.
 */
const
CHAR strFgHelpText[] =
        "\n"
        "Display the output of a job in the foreground.\n"
        "\n"
        "FG [-license] <id>\n";

/**
 Display usage text to the user.
 */
BOOL
FgHelp(VOID)
{
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("Fg %i.%02i\n"), YORI_VER_MAJOR, YORI_VER_MINOR);
#if YORI_BUILD_ID
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("  Build %i\n"), YORI_BUILD_ID);
#endif
    YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%hs"), strFgHelpText);
    return TRUE;
}

/**
 Entrypoint for the fg builtin command.

 @param ArgC The number of arguments.

 @param ArgV Array of arguments.

 @return ExitCode.
 */
DWORD
YORI_BUILTIN_FN
YoriCmd_FG(
    __in DWORD ArgC,
    __in YORI_STRING ArgV[]
    )
{
    DWORD JobId = 0;
    BOOL ArgumentUnderstood;
    DWORD i;
    DWORD StartArg = 0;
    YORI_STRING Arg;

    YoriLibLoadNtDllFunctions();
    YoriLibLoadKernel32Functions();

    for (i = 1; i < ArgC; i++) {

        ArgumentUnderstood = FALSE;
        ASSERT(YoriLibIsStringNullTerminated(&ArgV[i]));

        if (YoriLibIsCommandLineOption(&ArgV[i], &Arg)) {

            if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("?")) == 0) {
                FgHelp();
                return EXIT_SUCCESS;
            } else if (YoriLibCompareStringWithLiteralInsensitive(&Arg, _T("license")) == 0) {
                YoriLibDisplayMitLicense(_T("2017-2018"));
                return EXIT_SUCCESS;
            }
        } else {
            ArgumentUnderstood = TRUE;
            StartArg = i;
            break;
        }

        if (!ArgumentUnderstood) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("Argument not understood, ignored: %y\n"), &ArgV[i]);
        }
    }

    if (StartArg == 0) {
        YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("builtin: missing argument\n"));
        return EXIT_FAILURE;
    } else {

        HANDLE ReadPipe;
        HANDLE WritePipe;
        YORI_STRING Line;
        PVOID LineContext = NULL;
        LONGLONG llTemp;
        DWORD CharsConsumed;

        llTemp = 0;
        if (!YoriLibStringToNumber(&ArgV[StartArg], TRUE, &llTemp, &CharsConsumed)) {
            return EXIT_FAILURE;
        }

        JobId = (DWORD)llTemp;

        if (JobId == 0) {
            YoriLibOutput(YORI_LIB_OUTPUT_STDERR, _T("%y is not a valid job.\n"), &ArgV[StartArg]);
            return EXIT_FAILURE;
        }

        if (!CreatePipe(&ReadPipe, &WritePipe, NULL, 0)) {
            return EXIT_FAILURE;
        }

        if (!YoriCallPipeJobOutput(JobId, WritePipe, NULL)) {
            CloseHandle(ReadPipe);
            CloseHandle(WritePipe);
            return EXIT_FAILURE;
        }

        YoriLibCancelEnable(FALSE);
        YoriLibInitEmptyString(&Line);

        while (TRUE) {

            if (!YoriLibReadLineToString(&Line, &LineContext, ReadPipe)) {
                break;
            }
            YoriLibOutput(YORI_LIB_OUTPUT_STDOUT, _T("%y\n"), &Line);
            if (YoriLibIsOperationCancelled()) {
                break;
            }
        }

        YoriLibFreeStringContents(&Line);
        YoriLibCancelDisable();
        if (!YoriCallSetUnloadRoutine(YoriLibLineReadCleanupCache)) {
            YoriLibLineReadClose(LineContext);
        } else {
            YoriLibLineReadCloseOrCache(LineContext);
        }
        CloseHandle(ReadPipe);
    }
    return EXIT_SUCCESS;
}

// vim:sw=4:ts=4:et:
