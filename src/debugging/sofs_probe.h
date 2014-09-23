/**
 *  \file sofs_probe.h (interface file)
 *
 *  \brief A monitoring toolkit.
 *
 *  This toolkit provides a simple monitoring system which allows the programmer to include messages into his/her code.
 *  The system may be turned on or off.
 *  The system uses a probing depth to determine which messages must be displayed. The depth is a positive value.
 *  Upon writing the code, one should assign a depth to every probing message.
 *  Upon activating the probing system, one sets the range of depths that must be logged or displayed.
 *
 *  \author Artur Carneiro Pereira - September 2008
 *  \author Miguel Oliveira e Silva - September 2009
 *  \author António Rui Borges - July 2010
 *
 *  \remarks In case an error occurs, all functions return a negative value which is the symmetric of the system error
 *           that better represents the error cause.
 *           (execute command <em>man errno</em> to get the list of system errors)
 */

#ifndef SOFS_PROBE_H_
#define SOFS_PROBE_H_

#include <stdio.h>

/**
 *  \brief Opening of the probing system.
 *
 *  A call to this function does the following:
 *  \li if the probing system was previously closed, it is opened
 *  \li the output stream is assigned to the device whose reference is passed as argument
 *  \li if the probing system was opened by this call, the active depth range is set to [0,0].
 *
 *  \param fs the device to be assigned to the output stream
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the argument is \c NULL
 */

extern int soOpenProbe (FILE *fs);

/**
 *  \brief Closing the probing system.
 *
 *  A call to this function does the following:
 *  \li if the probing system was previously closed, nothing is done
 *  \li otherwise, the output stream is assigned to \c NULL and the active depth range is set to the initialized values.
 */

extern void soCloseProbe (void);

/**
 *  \brief Set probing depth.
 *
 *  A call to this function does the following:
 *  \li if the probing system was previously closed, it is opened
 *  \li the active range is set to the values passed as arguments
 *  \li if the probing system was opened by this call, the active output stream is set to \c stdout.
 *
 *  \param bottom the minimum probing depth to be activated
 *  \param top the maximum probing depth to be activated
 *
 *  \return <tt>0 (zero)</tt>, on success
 *  \return -\c EINVAL, if the range is invalid
 */

extern int soSetProbe (int bottom, int top);

/**
 *  \brief Print a probing message with the given depth.
 *
 *  Apart from the \e depth argument it works like the \e fprintf function.
 *
 *  \param depth the probing depth of the message
 *  \param fmt the format string (as in \e fprintf)
 *
 *  \return <em>the number of printed characters</em>, on success
 *  \return -\c EINVAL, if \e depth is invalid or \e fmt is \c NULL
 */

extern int soProbe (int depth, char *fmt, ...);

/**
 *  \brief Print a probing message with the given depth and color.
 *
 *  Apart from the \e depth argument, it works like the \e fprintf function.
 *  The color is a string in ANSI terminal format. For instance "07;31" means white font over red background.
 *
 *  \param depth the probing depth of the message
 *  \param color string defining the probing color
 *  \param fmt the format string (as in \e fprintf)
 *
 *  \return <em>the number of printed characters</em>, on success
 *  \return -\c EINVAL, if \e depth is invalid or \e color is \c NULL or \e fmt is \c NULL
 */

extern int soColorProbe (int depth, char *color, char *fmt, ...);

#endif /* SOFS_PROBE_H_ */
