package io.joynr.generator.cpp.communicationmodel
/*
 * !!!
 *
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.CompoundTypeTemplate
import io.joynr.generator.templates.util.NamingUtil
import javax.inject.Inject
import org.franca.core.franca.FCompoundType
import com.google.inject.assistedinject.Assisted

class TypeHTemplate extends CompoundTypeTemplate {

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension CppStdTypeUtil

	@Inject
	private extension NamingUtil

	@Inject
	private extension TemplateBase

	@Inject
	new(@Assisted FCompoundType type) {
		super(type)
	}

	override generate()
'''
«val typeName = type.joynrName»
«val headerGuard = ("GENERATED_TYPE_"+getPackagePathWithJoynrPrefix(type, "_", true)+"_"+typeName+"_H").toUpperCase»
«warning()»
#ifndef «headerGuard»
#define «headerGuard»

«getDllExportIncludeStatement()»

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <typeinfo>

#include "joynr/Util.h"
#include "joynr/TypeUtil.h"
#include "joynr/Variant.h"

// include complex Datatype headers.
«FOR member: type.typeDependencies»
	#include «member.includeOf»
«ENDFOR»

#include "joynr/serializer/Serializer.h"

«getNamespaceStarter(type, true)»

/**
«appendDoxygenSummaryAndWriteSeeAndDescription(type, " *")»
 * @version «majorVersion».«minorVersion»
 */
class «getDllExportMacro()» «typeName» «IF hasExtendsDeclaration(type)»: public «getExtendedType(type).typeName»«ENDIF»{

public:
	/**
	 * @brief MAJOR_VERSION The major version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::uint32_t MAJOR_VERSION;
	/**
	 * @brief MINOR_VERSION The minor version of this struct as specified in the
	 * type collection or interface in the Franca model.
	 */
	static const std::uint32_t MINOR_VERSION;

	// general methods

	// default constructor
	/** @brief Constructor */
	«typeName»();

	// constructor setting all fields
	«IF !getMembersRecursive(type).empty»
	/**
	 * @brief Parameterized constructor
	 «FOR member: getMembersRecursive(type)»
	 «appendDoxygenParameter(member, "*")»
	 «ENDFOR»
	 */
	explicit «typeName»(
			«FOR member: getMembersRecursive(type) SEPARATOR","»
				const «member.typeName» &«member.joynrName»
			«ENDFOR»
	);
	«ENDIF»

	/** @brief Copy constructor */
	«typeName»(const «typeName»&) = default;

	/** @brief Move constructor */
	«typeName»(«typeName»&&) = default;

	/** @brief Destructor */
	«IF !hasExtendsDeclaration(type)»
	virtual ~«typeName»() = default;
	«ELSE»
	~«typeName»() override = default;
	«ENDIF»

	/**
	 * @brief Stringifies the class
	 * @return stringified class content
	 */
	«IF !hasExtendsDeclaration(type)»
	virtual std::string toString() const;
	«ELSE»
	std::string toString() const override;
	«ENDIF»

	/**
	 * @brief Returns a hash code value for this object
	 * @return a hash code value for this object.
	 */
	 «IF !hasExtendsDeclaration(type)»
	 virtual std::size_t hashCode() const;
	«ELSE»
	 std::size_t hashCode() const override;
	«ENDIF»

	/**
	 * @brief assigns an object
	 * @return reference to the object assigned to
	 */
	«typeName»& operator=(const «typeName»&) = default;

	/**
	 * @brief move assigns an object
	 * @return reference to the object assigned to
	 */
	«typeName»& operator=(«typeName»&&) = default;

	«IF !hasExtendsDeclaration(type)»
	/**
	 * @brief "equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	bool operator==(const «typeName»& other) const
	{
	    if (typeid(*this) != typeid(other)) {
	        return false;
	    }
	    return this->equals(other);
	}

	/**
	 * @brief "not equal to" operator
	 * @param other reference to the object to compare to
	 * @return true if objects are not equal, false otherwise
	 */
	bool operator!=(const «typeName»& other) const
	{
	    return !(*this == other);
	}
	«ENDIF»

	/**
	 * @return a copy of this object
	 */
	 «IF !hasExtendsDeclaration(type)»
	 std::unique_ptr<«getRootType(type).joynrName»> virtual clone() const;
	 «ELSE»
	 std::unique_ptr<«getRootType(type).joynrName»> clone() const override;
	 «ENDIF»

	// getters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		/**
		 * @brief Gets «joynrName.toFirstUpper»
		 * @return «appendDoxygenComment(member, "* ")»
		 */
		inline const «member.typeName»& get«joynrName.toFirstUpper»() const { return «joynrName»; }
	«ENDFOR»

	// setters
	«FOR member: getMembers(type)»
		«val joynrName = member.joynrName»
		/**
		 * @brief Sets «joynrName.toFirstUpper»
		 «appendDoxygenParameter(member, "*")»
		 */
		inline void set«joynrName.toFirstUpper»(const «member.typeName»& «joynrName») { this->«joynrName» = «joynrName»; }
	«ENDFOR»

protected:
	// printing «typeName» with google-test and google-mock
	/**
	 * @brief Print values of a «typeName» object
	 * @param «typeName.toFirstLower» The current object instance
	 * @param os The output stream to send the output to
	 */
	friend void PrintTo(const «typeName»& «typeName.toFirstLower», ::std::ostream* os);

	/**
	 * @brief equals method
	 * @param other reference to the object to compare to
	 * @return true if objects are equal, false otherwise
	 */
	«IF hasExtendsDeclaration(type)»
		bool equals(const «getRootType(type).joynrName»& other) const override
		{
			«IF getMembers(type).size > 0»
				const «typeName»& otherDerived = static_cast<const «typeName»&>(other);
				return
				«FOR member: getMembers(type) SEPARATOR '&&'»
					this->«member.joynrName» == otherDerived.«member.joynrName»
				«ENDFOR»
				&& «getExtendedType(type).joynrName»::equals(other);
			«ELSE»
				return «getExtendedType(type).joynrName»::equals(other);
			«ENDIF»
		}
	«ELSE»
		virtual bool equals(const «typeName»& other) const
		{
			«IF getMembers(type).size > 0»
				return
				«FOR member: getMembers(type) SEPARATOR ' &&'»
					this->«member.joynrName» == other.«member.joynrName»
				«ENDFOR»
				;
			«ELSE»
				std::ignore = other;
				return true;
			«ENDIF»
		}
	«ENDIF»


«val serializeObjName = typeName.toLowerCase + "Obj"»
private:
	// serialize «typeName» with muesli
	template <typename Archive>
	friend void serialize(Archive& archive, «typeName»& «serializeObjName»);

	// members
	«FOR member: getMembers(type)»
		«member.typeName» «member.joynrName»;
		«IF isEnum(member.type)»
			std::string get«member.joynrName.toFirstUpper»Internal() const;
		«ENDIF»
	«ENDFOR»
};

std::size_t hash_value(const «typeName»& «typeName.toFirstLower»Value);

// serialize «typeName» with muesli
template <typename Archive>
void serialize(Archive& archive, «typeName»& «serializeObjName»)
{
«IF getMembers(type).size > 0 || hasExtendsDeclaration(type)»
	archive(
			«IF hasExtendsDeclaration(type)»
			muesli::BaseClass<«getExtendedType(type).joynrName»>(&«serializeObjName»)«IF type.members.size >0 »,«ENDIF»
			«ENDIF»
			«FOR member: type.members SEPARATOR ','»
			muesli::make_nvp("«member.joynrName»", «serializeObjName».«member.joynrName»)
			«ENDFOR»
	);
«ELSE»
	std::ignore = archive;
	std::ignore = «serializeObjName»;
«ENDIF»
}

«getNamespaceEnder(type, true)»

namespace std {

/**
 * @brief Function object that implements a hash function for «type.typeName».
 *
 * Used by the unordered associative containers std::unordered_set, std::unordered_multiset,
 * std::unordered_map, std::unordered_multimap as default hash function.
 */
template<>
struct hash<«type.typeName»> {

	/**
	 * @brief method overriding default implementation of operator ()
	 * @param «typeName.toFirstLower»Value the operators argument
	 * @return the ordinal number representing the enum value
	 */
	std::size_t operator()(const «type.typeName»& «typeName.toFirstLower»Value) const {
		return «type.buildPackagePath("::", true)»::hash_value(«typeName.toFirstLower»Value);
	}
};
} // namespace std

«val typeNameString = type.typeName.replace("::", ".")»
«IF type.hasExtendsDeclaration»
MUESLI_REGISTER_POLYMORPHIC_TYPE(«type.typeName», «getExtendedType(type).typeName», "«typeNameString»")
«ELSE»
MUESLI_REGISTER_TYPE(«type.typeName», "«typeNameString»")
«ENDIF»

#endif // «headerGuard»
'''
}
