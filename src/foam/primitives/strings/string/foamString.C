/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     | Version:     4.0
    \\  /    A nd           | Web:         http://www.foam-extend.org
     \\/     M anipulation  | For copyright notice see file Copyright
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "foamString.H"
#include "OSspecific.H"

/* * * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * */

const char* const Foam::string::typeName = "string";
int Foam::string::debug(debug::debugSwitchFromDict(string::typeName, 0));
const Foam::string Foam::string::null;

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

// Count and return the number of a given character in the string
Foam::string::size_type Foam::string::count(const char c) const
{
    register size_type cCount = 0;

    for (const_iterator iter = begin(); iter != end(); ++iter)
    {
        if (*iter == c)
        {
            cCount++;
        }
    }

    return cCount;
}


// Replace first occurence of sub-string oldStr with newStr
Foam::string& Foam::string::replace
(
    const string& oldStr,
    const string& newStr,
    size_type start
)
{
    size_type newStart = start;

    if ((newStart = find(oldStr, newStart)) != npos)
    {
        std::string::replace(newStart, oldStr.size(), newStr);
    }

    return *this;
}


// Replace all occurences of sub-string oldStr with newStr
Foam::string& Foam::string::replaceAll
(
    const string& oldStr,
    const string& newStr,
    size_type start
)
{
    if (oldStr.size())
    {
        size_type newStart = start;

        while ((newStart = find(oldStr, newStart)) != npos)
        {
            std::string::replace(newStart, oldStr.size(), newStr);
            newStart += newStr.size();
        }
    }

    return *this;
}


// Expand all occurences of environment variables and initial tilde sequences
Foam::string& Foam::string::expand()
{
    size_type startEnvar = 0;

    // Expand $VARS
    // Repeat until nothing more is found
    while
    (
        (startEnvar = find('$', startEnvar)) != npos
     && startEnvar < size()-1
    )
    {
        if (startEnvar == 0 || operator[](startEnvar-1) != '\\')
        {
            // Find end of first occurrence
            size_type endEnvar = startEnvar;
            size_type nd = 0;

            if (operator[](startEnvar+1) == '{')
            {
                endEnvar = find('}', startEnvar);
                nd = 1;
            }
            else
            {
                iterator iter = begin() + startEnvar + 1;

                while (iter != end() && (isalnum(*iter) || *iter == '_'))
                {
                    ++iter;
                    ++endEnvar;
                }
            }

            if (endEnvar != npos && endEnvar != startEnvar)
            {
                string enVar = substr
                (
                    startEnvar + 1 + nd,
                    endEnvar - startEnvar - 2*nd
                );

                string enVarString = getEnv(enVar);

                if (enVarString.size())
                {
                    std::string::replace
                    (
                        startEnvar,
                        endEnvar - startEnvar + 1,
                        enVarString
                    );
                    startEnvar += enVarString.size();
                }
                else
                {
                    //startEnvar = endEnvar;

                    FatalErrorIn("string::expand()")
                        << "Unknown variable name " << enVar << '.'
                        << exit(FatalError);
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            startEnvar++;
        }
    }

    if (size())
    {
        if (operator[](0) == '~')
        {
            // Expand initial ~
            //   ~/        => home directory
            //   ~foam => site/user FOAM configuration directory
            //   ~user     => home directory for specified user

            word user;
            fileName file;

            if ((startEnvar = find('/')) != npos)
            {
                user = substr(1, startEnvar - 1);
                file = substr(startEnvar + 1);
            }
            else
            {
                user = substr(1);
            }

            // NB: be a bit lazy and expand ~unknownUser as an
            // empty string rather than leaving it untouched.
            // otherwise add extra test
            if (user == "foam")
            {
                *this = findEtcFile(file);
            }
            else
            {
                *this = home(user)/file;
            }
        }
        else if (operator[](0) == '.')
        {
            // Expand initial '.' and './' into cwd
            if (size() == 1)
            {
                *this = cwd();
            }
            else if (operator[](1) == '/')
            {
                std::string::replace(0, 1, cwd());
            }
        }
    }

    return *this;
}


// Remove repeated characters returning true if string changed
bool Foam::string::removeRepeated(const char character)
{
    bool changed = false;

    if (character && find(character) != npos)
    {
        register string::size_type nChar=0;
        iterator iter2 = begin();

        register char prev = 0;

        for
        (
            string::const_iterator iter1 = iter2;
            iter1 != end();
            iter1++
        )
        {
            register char c = *iter1;

            if (prev == c && c == character)
            {
                changed = true;
            }
            else
            {
                *iter2 = prev = c;
                ++iter2;
                ++nChar;
            }
        }
        resize(nChar);
    }

    return changed;
}


// Return string with repeated characters removed
Foam::string Foam::string::removeRepeated(const char character) const
{
    string str(*this);
    str.removeRepeated(character);
    return str;
}


// Remove trailing character returning true if string changed
bool Foam::string::removeTrailing(const char character)
{
    bool changed = false;

    string::size_type nChar = size();
    if (character && nChar > 1 && operator[](nChar-1) == character)
    {
        resize(nChar-1);
        changed = true;
    }

    return changed;
}


// Return string with trailing character removed
Foam::string Foam::string::removeTrailing(const char character) const
{
    string str(*this);
    str.removeTrailing(character);
    return str;
}


// ************************************************************************* //
