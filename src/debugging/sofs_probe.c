/**
 *  \file sofs_probe.c (implementation file)
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
 */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

/*
 *  Internal data structure
 */

/** \brief Output stream */
static FILE *flog = NULL;
/** \brief Active probing depth: lower limit */
static int soLowerDepth = -1;
/** \brief Active probing depth: upper limit */
static int soHigherDepth = -1;

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

int soOpenProbe (FILE *fs)
{
  if (fs == NULL) return -EINVAL;                /* checking for null pointer */
  if (flog == NULL)
     { /* probing system is closed */
       flog = fs;                                /* set output stream to the device whose reference is passed as
                                                    argument */
       soLowerDepth = soHigherDepth = 0;         /* set range to default: [0,0] */
     }
     else { /* probing system is opened */
            fflush(flog);                        /* flush previous stream */
            flog = fs;                           /* set it to the device whose reference is passed as argument */
          }
  return 0;
}

/**
 *  \brief Closing the probing system.
 *
 *  A call to this function does the following:
 *  \li if the probing system was previously closed, nothing is done
 *  \li otherwise, the output stream is assigned to \c NULL and the active depth range is set to the initialized values.
 */

void soCloseProbe (void)
{
  if (flog != NULL)
     { flog = NULL;                              /* set output stream to NULL */
       soLowerDepth = soHigherDepth = -1;        /* set range to initialized values: [-1,-1] */
     }
}

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

int soSetProbe (int bottom, int top)
{
  if ((bottom < 0) || (top < bottom))            /* depth limits must be positive and top >= bottom */
     return -EINVAL;
  soLowerDepth = bottom;
  soHigherDepth = top;
  if (flog == NULL)
     flog = stdout;                              /* the probing system was previously closed, open it being \c stdout
                                                    the output stream */
  return 0;
}

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

int soProbe (int depth, char *fmt, ...)
{
  if (depth < 0 || fmt == NULL) return -EINVAL;  /* checking for arguments non conformity */
  if (flog == NULL) return 0;                    /* if the probing system is not opened, exit */
  if ((depth < soLowerDepth) || (depth > soHigherDepth))
     return 0;                                   /* if depth is not within the active range, exit */

  /* print the message */

  va_list ap;
  int n;

  fprintf (flog, "\e[07;34m(%d)-->\e[0m ", depth);
  n = (int) (depth/100);
  while (n-- > 0)
    fprintf(flog, "  ");
  va_start (ap, fmt);
  n = vfprintf (flog, fmt, ap);
  va_end (ap);
  return n;
}

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

int soColorProbe (int depth, char *color, char *fmt, ...)
{
  if (depth < 0 || color == NULL || fmt == NULL)
     return -EINVAL;  /* checking for arguments non conformity */
  if (flog == NULL) return 0;                    /* if the probing system is not opened, exit */
  if ((depth < soLowerDepth) || (depth > soHigherDepth))
     return 0;                                   /* if depth is not within the active range, exit */

  /* print the message */

  va_list ap;
  int n;

  fprintf (flog, "\e[%sm(%d)-->\e[0m ", color, depth);
  n = (int) (depth/100);
  while (n-- > 0)
    fprintf(flog, "  ");
  va_start (ap, fmt);
  n = vfprintf (flog, fmt, ap);
  va_end (ap);
  return n;
}
